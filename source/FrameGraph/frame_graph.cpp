#include "frame_graph.hpp"

bool _stageSupported(VkQueueFlags queue, VkPipelineStageFlags2 stage)
{
	if (queue == VK_QUEUE_TRANSFER_BIT)
	{
		stage &= ~VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		stage &= ~VK_PIPELINE_STAGE_2_RESOLVE_BIT;
		stage &= ~VK_PIPELINE_STAGE_2_COPY_BIT;
		stage &= ~VK_PIPELINE_STAGE_2_BLIT_BIT;
	}
	else if (queue == VK_QUEUE_COMPUTE_BIT)
	{
		stage &= ~VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	}
	else
	{
		stage = 0;
	}

	return stage == 0;
}

GFrameGraph::GFrameGraph(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{
}

GFrameGraph::~GFrameGraph()
{
	_clear();

	for (auto& stage : _stages)
		delete stage;
}

void GFrameGraph::_clear()
{
	_batches.clear();
	_ordererd.clear();
	_semaphores.clear();
	_ownershipsIn.clear();
	_ownershipsOut.clear();
}

void GFrameGraph::_recursive_register(IRenderStage* stage, std::unordered_map<IRenderStage*, std::unordered_set<IRenderStage*>>& adjacent_list)
{
	for (auto& adjacent : adjacent_list[stage])
		_recursive_register(adjacent, adjacent_list);

	if (std::find(_ordererd.begin(), _ordererd.end(), stage) == _ordererd.end())
	{
		_ordererd.push_back(stage);
	}
}

void GFrameGraph::_bind_semaphores(IRenderStage* stage, IRenderStage* next, const GStageViews::stage_resource& resource)
{
	VkPipelineStageFlags2 flags = resource.stage;

	if (!_stageSupported(next->Type, flags))
		flags = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

	if (resource.Op == EResourceOp::WriteOnly)
		flags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	std::shared_ptr<GVkSemaphore> semaphore = std::make_shared<GVkSemaphore>(Scope, flags, ESemaphoreFlags::InFlight);
	_semaphores[stage].signalSemaphores.push_back(semaphore);
	_semaphores[next].waitSemaphores.push_back(semaphore);
}

void GFrameGraph::_bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::stage_resource& resource)
{
	if (stage->Type == next->Type)
		return;

	VkPipelineStageFlags2 flags = resource.stage;
	VkAccessFlagBits2 access = resource.access;

	if (!_stageSupported(next->Type, flags))
	{
		access = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
		flags = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	}

	std::shared_ptr<GVkBarrier> barrier;
	if (resource.is_texture())
		barrier = std::make_shared<GVkBarrier>(static_cast<const GStageViews::image_resource&>(resource).view, flags, access, Scope->GetQueue(stage->Type).GetFamilyIndex(), Scope->GetQueue(next->Type).GetFamilyIndex());
	else
		barrier = std::make_shared<GVkBarrier>(static_cast<const GStageViews::buffer_resource&>(resource).view, flags, access, Scope->GetQueue(stage->Type).GetFamilyIndex(), Scope->GetQueue(next->Type).GetFamilyIndex());

	_ownershipsOut[stage].push_back(barrier);
	_ownershipsIn[next].push_back(barrier);
}

void GFrameGraph::_sync(std::vector<IRenderStage*>& list)
{
	if (list.empty())
		return;

	std::unordered_map<IRenderStage*, std::unordered_set<IRenderStage*>> _adjacentLists;

	for (auto& stage : list)
	{
		for (auto& other : list)
		{
			if (stage != other 
				&& !_adjacentLists[other].contains(stage) 
				&& !_adjacentLists[stage].contains(other))
			{
				if (stage->_dependsOn(*other))
					_adjacentLists[stage].insert(other);
			}
		}
	}

	for (auto& stage : list)
		_recursive_register(stage, _adjacentLists);

	for (size_t i = 0; i < _ordererd.size(); i++)
	{
		IRenderStage* stage = _ordererd[i];
		size_t j = (i + 1) % _ordererd.size();

		std::unordered_set<IVkObj*> visited = {};
		while (i != j)
		{
			IRenderStage* next = _ordererd[j];

			for (auto& resource : stage->Resources)
			{
				auto next_it = next->Resources.find_similar(resource);
				if (next_it != next->Resources.end() && !visited.contains(resource.ptr()))
				{
					_bind_semaphores(stage, next, resource);
					_bind_barriers(stage, next, resource);
					visited.insert(resource.ptr());
				}
			}

			j = (j + 1) % _ordererd.size();
		}
	}

	std::vector<IRenderStage*> batches{};
	VkQueueFlagBits lastQueue = VK_QUEUE_FLAG_BITS_MAX_ENUM;
	std::shared_ptr<GVkFence> batchFence = std::make_shared<GVkFence>(Scope, EFenceFlags::CreateSignaled | EFenceFlags::InFlight);

	for (auto& stage : _ordererd)
	{
		if (lastQueue != stage->Type && !batches.empty())
		{
			_batches.emplace_back(lastQueue, batchFence, std::move(batches));

			batchFence = std::make_shared<GVkFence>(Scope, EFenceFlags::CreateSignaled | EFenceFlags::InFlight);
			batches.clear();
		}

		stage->CommandBuffer->SetFence(batchFence);
		batches.push_back(stage);
		lastQueue = stage->Type;
	}

	if (!batches.empty())
		_batches.emplace_back(lastQueue, batchFence, std::move(batches));
}

void GFrameGraph::_submit_batch(const recording_batch& batch)
{
	std::vector<submit_descriptor> submitDescriptors(batch.stages.size());
	std::vector<VkSubmitInfo> submitInfos(batch.stages.size());

	uint32_t index = 0;
	for (auto& stage : batch.stages)
	{
		submitDescriptors[index].cmd = stage->CommandBuffer->GetCommandBuffer();

		for (auto& semaphore : _semaphores[stage].waitSemaphores)
		{
			if (auto sem = semaphore->GetWaitSemaphore())
			{
				submitDescriptors[index].waitStages.push_back(semaphore->GetPipelineStage());
				submitDescriptors[index].waitSemaphores.push_back(sem);
			}
		}

		for (auto& semaphore : _semaphores[stage].signalSemaphores)
		{
			if (auto sem = semaphore->GetSignalSemaphore())
				submitDescriptors[index].signalSemaphores.push_back(sem);
		}

		submitInfos[index].commandBufferCount = 1;
		submitInfos[index].pCommandBuffers = &submitDescriptors[index].cmd;

		submitInfos[index].signalSemaphoreCount = submitDescriptors[index].signalSemaphores.size();
		submitInfos[index].pSignalSemaphores = submitDescriptors[index].signalSemaphores.data();

		submitInfos[index].waitSemaphoreCount = submitDescriptors[index].waitSemaphores.size();
		submitInfos[index].pWaitSemaphores = submitDescriptors[index].waitSemaphores.data();
		submitInfos[index].pWaitDstStageMask = submitDescriptors[index].waitStages.data();

		submitInfos[index].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		index++;
	}

	batch.fence->Reset();
	Scope->GetQueue(batch.queueFlags).submit(submitInfos, batch.fence->GetFence());
}

void GFrameGraph::Draw(const GCamera& Camera, const IWorld& World)
{
	if (_revision != World.GetDrawableRevisionIndex())
	{
		for (auto& stage : _stages)
			stage->Reset(World);

		_revision = World.GetDrawableRevisionIndex();
		_flags = 0;
	}

	std::vector<IRenderStage*> _activeStages;
	_activeStages.reserve(_stages.size());

	size_t new_flags = 0;
	for (size_t i = 0; i < _stages.size(); i++)
	{
		if (_stages[i]->IsActive(World))
		{
			_activeStages.emplace_back(_stages[i]);
			new_flags |= 1ull << i;
		}
	}

	if (new_flags != _flags)
	{
		_clear();
		_flags = new_flags;

		_sync(_activeStages);
	}

	for (auto& batch : _batches)
	{
		batch.fence->Wait();

		for (auto& stage : batch.stages)
		{
			stage->_bind_acquire_barriers(_ownershipsIn[stage]);

			stage->record(Camera, World);

			stage->_bind_release_barriers(_ownershipsOut[stage]);
		}

		_submit_batch(batch);
	}
}