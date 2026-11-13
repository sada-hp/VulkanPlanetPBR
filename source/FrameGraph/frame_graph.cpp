#include "frame_graph.hpp"

GFrameGraph::GFrameGraph(std::shared_ptr<RenderScope> InScope)
	: m_Scope(InScope)
{
}

GFrameGraph::~GFrameGraph()
{
	_clear();
}

void GFrameGraph::_clear()
{
	m_batches.clear();
	m_semaphores.clear();
	m_ownershipsIn.clear();
	m_ownershipsOut.clear();
}

void GFrameGraph::_recursive_register(IRenderStage* stage, std::unordered_map<IRenderStage*, adjacent_data>& adjacent_list, std::vector<IRenderStage*>& target_list)
{
	if (adjacent_list[stage].owner_state == e_dfs_state::unvisited)
	{
		adjacent_list[stage].owner_state = e_dfs_state::active;

		for (auto& adjacent : adjacent_list[stage].list)
		{
			auto state = adjacent_list[adjacent].owner_state;

			assert(state != e_dfs_state::active);
			_recursive_register(adjacent, adjacent_list, target_list);
		}

		adjacent_list[stage].owner_state = e_dfs_state::done;
		target_list.push_back(stage);
	}
}

void GFrameGraph::_reorder_for_async(const std::unordered_map<IRenderStage*, adjacent_data>& adjacent_list, std::vector<IRenderStage*>& target_list)
{
	std::vector<IRenderStage*> _asyncPasses = {};
	std::vector<IRenderStage*> _otherPasses = {};

	for (auto& Stage : target_list)
	{
		bool waitsOnGQ = Stage->Type == VK_QUEUE_GRAPHICS_BIT;

		if (!waitsOnGQ)
		{
			for (auto& adjacent : adjacent_list.at(Stage).list)
			{
				if (adjacent->Type == VK_QUEUE_GRAPHICS_BIT)
				{
					waitsOnGQ = true;
					break;
				}
			}
		}

		if (!waitsOnGQ)
			_asyncPasses.push_back(Stage);
		else
			_otherPasses.push_back(Stage);
	}

	target_list.clear();
	target_list.insert(target_list.end(), _asyncPasses.begin(), _asyncPasses.end());
	target_list.insert(target_list.end(), _otherPasses.begin(), _otherPasses.end());
}

void GFrameGraph::_bind_semaphores(IRenderStage* stage, IRenderStage* next, const GStageViews::stage_resource& resource)
{
	VkPipelineStageFlags2 flags = resource.stage;

	if (!m_Scope->GetQueue(next->Type).IsStageSupported(flags))
		flags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	std::shared_ptr<GVkSemaphore> semaphore = std::make_shared<GVkSemaphore>(m_Scope, flags, ESemaphoreFlags::InFlight);
	m_semaphores[stage].signalSemaphores.push_back(semaphore);
	m_semaphores[next].waitSemaphores.push_back(semaphore);
}

void GFrameGraph::_bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::stage_resource& resource)
{
	VkPipelineStageFlags2 flags = resource.stage;
	VkAccessFlagBits2 access = resource.access;

	if (!m_Scope->GetQueue(next->Type).IsStageSupported(flags))
	{
		access = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
		flags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}

	std::shared_ptr<GVkBarrier> barrier;
	if (resource.is_texture())
		barrier = std::make_shared<GVkBarrier>(static_cast<const GStageViews::image_resource&>(resource).view, flags, access, m_Scope->GetQueue(stage->Type).GetFamilyIndex(), m_Scope->GetQueue(next->Type).GetFamilyIndex());
	else
		barrier = std::make_shared<GVkBarrier>(static_cast<const GStageViews::buffer_resource&>(resource).view, flags, access, m_Scope->GetQueue(stage->Type).GetFamilyIndex(), m_Scope->GetQueue(next->Type).GetFamilyIndex());

	m_ownershipsOut[stage].push_back(barrier);
	m_ownershipsIn[next].push_back(barrier);
}

void GFrameGraph::_sync(std::vector<IRenderStage*>& list)
{
	if (list.empty())
		return;

	std::unordered_map<IRenderStage*, adjacent_data> _adjacentLists;

	for (auto& stage : list)
	{
		for (auto& other : list)
		{
			if (stage != other 
				&& !_adjacentLists[other].list.contains(stage) 
				&& !_adjacentLists[stage].list.contains(other))
			{
				if (stage->_dependsOn(*other))
					_adjacentLists[stage].list.insert(other);
			}
		}
	}

	std::vector<IRenderStage*> ordered;
	for (auto& stage : list)
		_recursive_register(stage, _adjacentLists, ordered);

	_reorder_for_async(_adjacentLists, ordered);

	for (size_t i = 0; i < ordered.size(); i++)
	{
		IRenderStage* stage = ordered[i];
		size_t j = (i + 1) % ordered.size();

		std::unordered_set<IVkObj*> visited = {};
		while (i != j)
		{
			IRenderStage* next = ordered[j];

			for (auto& resource : stage->Resources)
			{
				auto next_it = next->Resources.find_similar(resource);
				if (next_it != next->Resources.end() && !visited.contains(resource.ptr()))
				{
					if (stage->Type != next->Type)
					{
						_bind_semaphores(stage, next, resource);
						_bind_barriers(stage, next, resource);
					}

					visited.insert(resource.ptr());
				}
			}

			j = (j + 1) % ordered.size();
		}
	}

	std::vector<IRenderStage*> batches{};
	VkQueueFlagBits lastQueue = VK_QUEUE_FLAG_BITS_MAX_ENUM;
	std::shared_ptr<GVkFence> batchFence = std::make_shared<GVkFence>(m_Scope, EFenceFlags::CreateSignaled | EFenceFlags::InFlight);

	for (auto& stage : ordered)
	{
		if (lastQueue != stage->Type && !batches.empty())
		{
			m_batches.emplace_back(lastQueue, batchFence, std::move(batches));

			batchFence = std::make_shared<GVkFence>(m_Scope, EFenceFlags::CreateSignaled | EFenceFlags::InFlight);
			batches.clear();
		}

		stage->CommandBuffer->SetFence(batchFence);
		batches.push_back(stage);
		lastQueue = stage->Type;
	}

	if (!batches.empty())
		m_batches.emplace_back(lastQueue, batchFence, std::move(batches));
}

void GFrameGraph::_submit_batch(const recording_batch& batch)
{
	std::vector<submit_descriptor> submitDescriptors(batch.stages.size());
	std::vector<VkSubmitInfo> submitInfos(batch.stages.size());

	uint32_t index = 0;
	for (auto& stage : batch.stages)
	{
		submitDescriptors[index].cmd = stage->CommandBuffer->GetCommandBuffer();

		for (auto& semaphore : m_semaphores[stage].waitSemaphores)
		{
			if (auto sem = semaphore->GetWaitSemaphore())
			{
				submitDescriptors[index].waitStages.push_back(semaphore->GetPipelineStage());
				submitDescriptors[index].waitSemaphores.push_back(sem);
			}
		}

		for (auto& semaphore : m_semaphores[stage].signalSemaphores)
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
	m_Scope->GetQueue(batch.queueFlags).submit(submitInfos, batch.fence->GetFence());
}

void GFrameGraph::Draw(const GCamera& Camera, const IWorld& World)
{
	if (m_revision != World.GetDrawableRevisionIndex())
	{
		for (auto& stage : m_stages)
			stage->Reset(World);

		m_revision = World.GetDrawableRevisionIndex();
		m_flags = 0;
	}

	std::vector<IRenderStage*> _activeStages;
	_activeStages.reserve(m_stages.size());

	size_t new_flags = 0;
	for (size_t i = 0; i < m_stages.size(); i++)
	{
		if (m_stages[i]->IsActive(World))
		{
			_activeStages.emplace_back(m_stages[i].get());
			new_flags |= 1ull << i;
		}
	}

	if (new_flags != m_flags)
	{
		_clear();
		m_flags = new_flags;

		_sync(_activeStages);
	}

	for (auto& batch : m_batches)
	{
		for (auto& stage : batch.stages)
		{
			stage->_bind_acquire_barriers(m_ownershipsIn[stage]);

			stage->record(Camera, World);

			stage->_bind_release_barriers(m_ownershipsOut[stage]);
		}

		_submit_batch(batch);
	}
}