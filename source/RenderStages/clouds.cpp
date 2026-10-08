#include "clouds.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"

GCloudsStage::GCloudsStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	Images.PerlinWorley = _generate_noise(GShaderNoise::PerlinWorleyPS, VK_FORMAT_R32_SFLOAT, VkExtent3D{ 128, 128, 128 }, 6, 16);
	Images.HighFrequency = _generate_noise(GShaderNoise::WorleyPS, VK_FORMAT_R32G32B32A32_SFLOAT, VkExtent3D{ 64, 64, 64 }, 4, 4);

	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(Resources.ColorBuffer->GetFormat(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ Resources.ColorBuffer });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(Resources.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.IrradianceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.ScatteringLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.PerlinWorley), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.HighFrequency), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.SetRenderPass(RenderPass);
	PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AppendCode(GShaderUtils::NoiseCommon)
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderUtils::UtilsCommon)
		.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaders::CloudsPS);

	PSODesc.AttachmentBlendState(0).blendEnable = VK_TRUE;
	PSODesc.AttachmentBlendState(0).colorBlendOp = VK_BLEND_OP_ADD;
	PSODesc.AttachmentBlendState(0).srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	PSODesc.AttachmentBlendState(0).dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	Pipeline = PSODesc.Construct(Scope);
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

	std::shared_ptr<GVkImage> Image = GVkImageFactory::Image(Scope, Format, extents, EImageFlags::Sampler | EImageFlags::RenderTarget);

	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Format);
	std::shared_ptr<GVkRenderPass> _renderPass = RPDesc.Construct(Scope);
	std::shared_ptr<GVkFramebuffer> _framebuffer = std::make_shared<GVkFramebuffer>(Scope, _renderPass, std::vector{ GVkImage::ToView(Image, extents.depth > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D) });
	
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
	_commandBuffer->Submit();

	return Image;
}

void GCloudsStage::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->BindDescriptorSet(0, DescriptorSet);
	CommandBuffer->Draw(3);
	CommandBuffer->Submit();
}