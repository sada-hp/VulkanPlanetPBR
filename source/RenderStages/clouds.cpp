#include "clouds.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"

GCloudsStage::GCloudsStage(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	Images.PerlinWorley = _generate_noise(GShaderNoise::PerlinWorleyPS, VK_FORMAT_R16_SFLOAT, VkExtent3D{ 128, 128, 128 }, 8, 5);
	Images.HighFrequency = _generate_noise(GShaderNoise::WorleyPS, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 64, 64, 64 }, 6, 3);

	VkExtent2D ExtentLR = { GVkSharedResources::Instance.ColorBuffer->GetExtent().width / 2, GVkSharedResources::Instance.ColorBuffer->GetExtent().height / 2 };
	Images.CloudsLR = GVkImageFactory::ColorRenderTargetFlight(Scope, ExtentLR);
	Images.DepthLR = GVkImageFactory::DepthRenderTargetFlight(Scope, ExtentLR);

	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentClearOp(Images.CloudsLR->GetFormat(), VkClearValue{}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	RPDesc.AddAttachmentClearOp(Images.DepthLR->GetFormat());
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ Images.CloudsLR, Images.DepthLR });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(GVkSharedResources::Instance.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.IrradianceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.ScatteringLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.PerlinWorley), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.HighFrequency), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToFlightHadle(Images.CloudsLR, -1), GVkSamplerFactory::PointSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToFlightHadle(Images.DepthLR, -1), GVkSamplerFactory::PointSamplerClamp(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);

	// under cloud layer
	{
		GraphicsPipelineDescriptor PSODesc{};
		PSODesc.SetRenderPass(RenderPass);
		PSODesc.AddPushConstants<int>(VK_SHADER_STAGE_FRAGMENT_BIT);
		PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
		PSODesc.VS.AppendCode(GShaders::FullscreenVS);
		PSODesc.PS.AppendCode(GShaderUtils::NoiseCommon)
			.AppendCode(GShaderUtils::LightingCommon)
			.AppendCode(GShaderUtils::UtilsCommon)
			.AppendCode(GShaderUtils::UBOCommon)
			.AppendCode(GShaders::CloudsCommonPS)
			.AppendCode(GShaders::CloudsSetupUnderPS)
			.AppendCode(GShaders::CloudsPS);

		Pipelines.Under = PSODesc.Construct(Scope);
	}

	// inside cloud layer
	{
		GraphicsPipelineDescriptor PSODesc{};
		PSODesc.SetRenderPass(RenderPass);
		PSODesc.AddPushConstants<int>(VK_SHADER_STAGE_FRAGMENT_BIT);
		PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
		PSODesc.VS.AppendCode(GShaders::FullscreenVS);
		PSODesc.PS.AppendCode(GShaderUtils::NoiseCommon)
			.AppendCode(GShaderUtils::LightingCommon)
			.AppendCode(GShaderUtils::UtilsCommon)
			.AppendCode(GShaderUtils::UBOCommon)
			.AppendCode(GShaders::CloudsCommonPS)
			.AppendCode(GShaders::CloudsSetupBetweenPS)
			.AppendCode(GShaders::CloudsPS);

		Pipelines.Between = PSODesc.Construct(Scope);
	}

	// above cloud layer
	{
		GraphicsPipelineDescriptor PSODesc{};
		PSODesc.SetRenderPass(RenderPass);
		PSODesc.AddPushConstants<int>(VK_SHADER_STAGE_FRAGMENT_BIT);
		PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
		PSODesc.VS.AppendCode(GShaders::FullscreenVS);
		PSODesc.PS.AppendCode(GShaderUtils::NoiseCommon)
			.AppendCode(GShaderUtils::LightingCommon)
			.AppendCode(GShaderUtils::UtilsCommon)
			.AppendCode(GShaderUtils::UBOCommon)
			.AppendCode(GShaders::CloudsCommonPS)
			.AppendCode(GShaders::CloudsSetupAbovePS)
			.AppendCode(GShaders::CloudsPS);

		Pipelines.Above = PSODesc.Construct(Scope);
	}
}

GCloudsStage::~GCloudsStage()
{

}

std::shared_ptr<GVkImage> GCloudsStage::_generate_noise(const std::string& shader, VkFormat Format, VkExtent3D extents, uint32_t freq, uint32_t octaves)
{
	struct Settings
	{
		uint32_t lyr;
		uint32_t freq;
		uint32_t octaves;
		uint32_t seed;
	};

	std::shared_ptr<GVkImage> Image = GVkImageFactory::Image(Scope, Format, extents, EImageFlags::TransferSource | EImageFlags::TransferTarget | EImageFlags::Sampler | EImageFlags::AllocateMipMaps | EImageFlags::RenderTarget);

	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Format);
	std::shared_ptr<GVkRenderPass> _renderPass = RPDesc.Construct(Scope);
	std::shared_ptr<GVkFramebuffer> _framebuffer = std::make_shared<GVkFramebuffer>(Scope, _renderPass, std::vector{ GVkImage::ToView(Image, 0, 0, 1, VK_REMAINING_ARRAY_LAYERS, extents.depth > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D) });
	
	GraphicsPipelineDescriptor PSODesc{};

	PSODesc.PS.AppendCode(GShaderUtils::NoiseCommon);

	if (extents.depth > 1)
	{
		PSODesc.VS.AppendCode(GShaders::FullscreenLayeredVS);
		PSODesc.GS.AppendCode(GShaderNoise::NoiseGS);
		PSODesc.PS.AddDefine("_3D_NOISE");

		PSODesc.AddPushConstants<Settings>(VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		PSODesc.VS.AppendCode(GShaders::FullscreenVS);
		PSODesc.AddPushConstants<Settings>(VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	PSODesc.PS.AppendCode(shader);
	PSODesc.SetRenderPass(_renderPass);
	std::shared_ptr<GVkPipeline> _pipeline = PSODesc.Construct(Scope);

	std::shared_ptr<GVkCommandBuffer> _commandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT);
	Settings _settings{ extents.depth, freq, octaves, 42u };

	_commandBuffer->BeginRenderPass(_renderPass, _framebuffer);
	_commandBuffer->BindPipeline(_pipeline);
	_commandBuffer->PushConstants(0, _settings);
	_commandBuffer->Draw(3, extents.depth);
	_commandBuffer->EndRenderPass();
	_commandBuffer->GenerateMipMaps(Image);
	_commandBuffer->Submit();

	return Image;
}

void GCloudsStage::Execute(const GCamera& Camera, const IWorld& World)
{
	static int Order = 0;

	// need to connect with shader
	float Rg = 6360 * 1e3;
	float Rt = 6420 * 1e3;
	float BottomBound = Rg + 0.25 * (Rt - Rg);
	float TopBound    = Rg + 0.60 * (Rt - Rg);

	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);

	float CameraHeight = glm::length(Camera.GetWorldMatrix().GetPosition());

	if (CameraHeight < BottomBound)
		CommandBuffer->BindPipeline(Pipelines.Under);
	else if (CameraHeight > TopBound)
		CommandBuffer->BindPipeline(Pipelines.Above);
	else
		CommandBuffer->BindPipeline(Pipelines.Between);

	CommandBuffer->BindDescriptorSet(0, DescriptorSet);
	CommandBuffer->PushConstants(0, Order);

	CommandBuffer->Draw(3);
	CommandBuffer->EndRenderPass();

	auto view = GVkImage::ToView(Images.CloudsLR);
	CommandBuffer->BlitImage(view, GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer));
	CommandBuffer->BindBarrier<GVkShaderReadBarrier>(view, VK_SHADER_STAGE_FRAGMENT_BIT);

	CommandBuffer->Submit();
	Order++;
}