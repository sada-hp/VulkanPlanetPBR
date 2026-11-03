#include "frame_graph.hpp"

GFrameGraph::GFrameGraph(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{
}

GFrameGraph::~GFrameGraph()
{
	for (auto& stage : _stages)
		delete stage;
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

void GFrameGraph::_bind_semaphores(IRenderStage* stage, IRenderStage* next, VkPipelineStageFlags flags)
{
	std::shared_ptr<GVkSemaphore> semaphore = std::make_shared<GVkSemaphore>(Scope, flags, ESemaphoreFlags::InFlight);
	_semaphores[stage].signalSemaphores.push_back(semaphore);
	_semaphores[next].waitSemaphores.push_back(semaphore);
}

void GFrameGraph::_bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::buffer_iterator& it)
{
	std::shared_ptr<GVkBarrier> barrier = std::make_shared<GVkBarrier>(it->view, it->stage, it->access, Scope->GetQueue(stage->Type).GetFamilyIndex(), Scope->GetQueue(next->Type).GetFamilyIndex());
	_ownershipsOut[stage].push_back(barrier);
	_ownershipsIn[next].push_back(barrier);
}

void GFrameGraph::_bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::image_iterator& it)
{
	std::shared_ptr<GVkBarrier> barrier = std::make_shared<GVkBarrier>(it->view, Scope->GetQueue(stage->Type).GetFamilyIndex(), Scope->GetQueue(next->Type).GetFamilyIndex());
	_ownershipsOut[stage].push_back(barrier);
	_ownershipsIn[next].push_back(barrier);
}

void GFrameGraph::_sync(std::vector<IRenderStage*>& list)
{
	_ordererd.clear();
	_semaphores.clear();
	_ownershipsIn.clear();
	_ownershipsOut.clear();

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

		while (i != j)
		{
			IRenderStage* next = _ordererd[j];

			if (next->Type != stage->Type)
			{
				for (auto it = stage->Resources.image_begin(); it != stage->Resources.image_end(); it++)
				{
					auto next_it = next->Resources.find(it->view);
					if (next->Resources.is_valid(next_it))
					{
						_bind_semaphores(stage, next, it->stage);
						_bind_barriers(stage, next, it);
					}
				}

				for (auto it = stage->Resources.buffer_begin(); it != stage->Resources.buffer_end(); it++)
				{
					auto next_it = next->Resources.find(it->view);
					if (next->Resources.is_valid(next_it))
					{
						_bind_semaphores(stage, next, it->stage);
						_bind_barriers(stage, next, it);
					}
				}
			}

			j = (j + 1) % _ordererd.size();
		}
	}
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
		_sync(_activeStages);
		_flags = new_flags;
	}

	for (auto& stage : _ordererd)
	{
		stage->_acquire_resources(_ownershipsIn[stage]);

		stage->record(Camera, World);

		stage->_submit(_ownershipsOut[stage], _semaphores[stage].waitSemaphores, _semaphores[stage].signalSemaphores);
	}
}