#pragma once
#include "RenderStages/IRenderStage.hpp"
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/render_pass.hpp"
#include "VulkanAPI/framebuffer.hpp"
#include "VulkanAPI/pipeline.hpp"
#include "VulkanAPI/scope.hpp"

class GAtmospherePrecomputeLUT
{
	struct GLutPass
	{
		std::shared_ptr<GVkRenderPass>  RenderPass = VK_NULL_HANDLE;
		std::shared_ptr<GVkFramebuffer> Framebuffer = VK_NULL_HANDLE;
		std::shared_ptr<GVkPipeline> Pipeline = VK_NULL_HANDLE;
		std::shared_ptr<GVkDescriptorSet> Set = VK_NULL_HANDLE;
		std::vector<int> PushConstants = {};
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<GLutPass> m_precomputePasses = {};

private:
	struct
	{
		std::shared_ptr<GVkImage> DeltaJ = VK_NULL_HANDLE;
		std::shared_ptr<GVkImage> DeltaSR = VK_NULL_HANDLE;
		std::shared_ptr<GVkImage> DeltaSM = VK_NULL_HANDLE;
	} Images;

private:
	GLutPass _addIrradiancePass(GVkSharedResources& Resources);
	GLutPass _addTransmittancePass(GVkSharedResources& Resources);
	GLutPass _addIrradianceMultiStep(GVkSharedResources& Resources);
	GLutPass _addSignleScatteringPass(GVkSharedResources& Resources);

	GLutPass _addScatteringMultiEvalStep(GVkSharedResources& Resources);
	GLutPass _addScatteringMultiAddStep(GVkSharedResources& Resources);

	void _initImages(GVkSharedResources& Resources);

	GAtmospherePrecomputeLUT(std::shared_ptr<RenderScope> Scope, GVkSharedResources& Resources);
	virtual ~GAtmospherePrecomputeLUT() {}
	
	void _executePass(std::shared_ptr<GVkCommandBuffer> CommandBuffer, GLutPass& pass);
	void _runPrecompute();

public:
	static void Execute(std::shared_ptr<RenderScope> Scope, GVkSharedResources& Resources);
};