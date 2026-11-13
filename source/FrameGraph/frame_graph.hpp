#pragma once
#include "RenderStages/IRenderStage.hpp"
#include <unordered_map>
#include <unordered_set>

class GFrameGraph
{
	enum class e_dfs_state
	{
		unvisited,
		active,
		done
	};

	struct submit_descriptor
	{
		VkCommandBuffer cmd = VK_NULL_HANDLE;
		std::vector<VkPipelineStageFlags> waitStages{};
		std::vector<VkSemaphore> waitSemaphores{}, signalSemaphores{};
	};

	struct semaphore_pair
	{
		std::vector<std::shared_ptr<GVkSemaphore>> waitSemaphores;
		std::vector<std::shared_ptr<GVkSemaphore>> signalSemaphores;
	};

	struct recording_batch
	{
		VkQueueFlagBits queueFlags;
		std::shared_ptr<GVkFence> fence;
		std::vector<IRenderStage*> stages;
	};

	struct adjacent_data
	{
		e_dfs_state owner_state = e_dfs_state::unvisited;
		std::unordered_set<IRenderStage*> list;
	};

private:
	std::unordered_map<IRenderStage*, std::vector<std::shared_ptr<GVkBarrier>>> m_ownershipsOut;
	std::unordered_map<IRenderStage*, std::vector<std::shared_ptr<GVkBarrier>>> m_ownershipsIn;
	std::unordered_map<IRenderStage*, semaphore_pair> m_semaphores;
	std::vector<recording_batch> m_batches;

private:
	std::vector<std::unique_ptr<IRenderStage>> m_stages;
	size_t m_revision = ~0;
	size_t m_flags = 0;

protected:
	std::shared_ptr<RenderScope> m_Scope = VK_NULL_HANDLE;

private:
	void _clear();
	void _sync(std::vector<IRenderStage*>& list);
	void _bind_barriers(IRenderStage* stage, IRenderStage* next, const GStageViews::stage_resource& resource);
	void _bind_semaphores(IRenderStage* stage, IRenderStage* next, const GStageViews::stage_resource& resource);

	void _reorder_for_async(const std::unordered_map<IRenderStage*, adjacent_data>& adjacent_list, std::vector<IRenderStage*>& target_list);
	void _recursive_register(IRenderStage* stage, std::unordered_map<IRenderStage*, adjacent_data>& adjacent_list, std::vector<IRenderStage*>& target_list);

	void _submit_batch(const recording_batch& batch);

public:
	GFrameGraph(std::shared_ptr<RenderScope> Scope);
	~GFrameGraph();

	void Draw(const GCamera& Camera, const IWorld& World);

public:
	template<typename T, typename ...Args>
	void AddStage(Args&& ...args)
	{
		m_stages.push_back(std::make_unique<T>(args...));
	}
};