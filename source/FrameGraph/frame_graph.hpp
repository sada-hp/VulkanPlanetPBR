#pragma once
#include "RenderStages/IRenderStage.hpp"
#include <unordered_map>
#include <unordered_set>

class GFrameGraph
{
	struct semaphore_pair
	{
		std::vector<std::shared_ptr<GVkSemaphore>> waitSemaphores;
		std::vector<std::shared_ptr<GVkSemaphore>> signalSemaphores;
	};

private:
	std::unordered_map<IRenderStage*, std::vector<std::shared_ptr<GVkBarrier>>> _ownershipsOut;
	std::unordered_map<IRenderStage*, std::vector<std::shared_ptr<GVkBarrier>>> _ownershipsIn;
	std::unordered_map<IRenderStage*, semaphore_pair> _semaphores;
	std::vector<IRenderStage*> _ordererd;

private:
	std::vector<IRenderStage*> _stages;
	size_t _revision = ~0;
	size_t _flags = 0;

protected:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

private:
	void _recursive_register(IRenderStage* stage, std::unordered_map<IRenderStage*, std::unordered_set<IRenderStage*>>& adjacent_list);
	
	void _bind_semaphores(IRenderStage* stage, IRenderStage* next, VkPipelineStageFlags flags);
	void _bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::image_iterator& it);
	void _bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::buffer_iterator& it);

	void _sync(std::vector<IRenderStage*>& list);

public:
	GFrameGraph(std::shared_ptr<RenderScope> Scope);
	~GFrameGraph();

	void Draw(const GCamera& Camera, const IWorld& World);

public:
	template<typename T, typename ...Args>
	void AddStage(Args&& ...args)
	{
		_stages.push_back(new T(args...));
	}
};