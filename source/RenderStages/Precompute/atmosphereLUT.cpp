#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"
#include "atmosphereLUT.hpp"

GAtmospherePrecomputeLUT::GLutPass GAtmospherePrecomputeLUT::_addTransmittancePass(GVkSharedResources& Resources)
{
	GLutPass Pass{};

	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Resources.TransmittanceLUT->GetFormat());
	Pass.RenderPass = RPDesc.Construct(Scope);
	Pass.Framebuffer = std::make_shared<GVkFramebuffer>(Scope, Pass.RenderPass, std::vector{ Resources.TransmittanceLUT });

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AddDefine("_ATMO_USE_KILOMETERS")
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderPrecomputeLUT::LUTCommon)
		.AppendCode(GShaderPrecomputeLUT::TransmittancePS);
	PSODesc.SetRenderPass(Pass.RenderPass);
	Pass.Pipeline = PSODesc.Construct(Scope);

	return Pass;
}

GAtmospherePrecomputeLUT::GLutPass GAtmospherePrecomputeLUT::_addIrradiancePass(GVkSharedResources& Resources)
{
	GLutPass Pass{};
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Resources.IrradianceLUT->GetFormat());
	Pass.RenderPass = RPDesc.Construct(Scope);

	Pass.Framebuffer = std::make_shared<GVkFramebuffer>(Scope, Pass.RenderPass, std::vector{ Resources.IrradianceLUT });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	Pass.Set = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AddDefine("_ATMO_USE_KILOMETERS")
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderPrecomputeLUT::LUTCommon)
		.AppendCode(GShaderPrecomputeLUT::IrradiancePS);
	PSODesc.AddDescriptorLayout(Pass.Set->GetLayout());
	PSODesc.SetRenderPass(Pass.RenderPass);
	Pass.Pipeline = PSODesc.Construct(Scope);

	return Pass;
}

GAtmospherePrecomputeLUT::GLutPass GAtmospherePrecomputeLUT::_addSignleScatteringPass(GVkSharedResources& Resources)
{
	GLutPass Pass{};

	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Images.DeltaSR->GetFormat());
	RPDesc.AddAttachmentDontCareOp(Images.DeltaSM->GetFormat());
	RPDesc.AddAttachmentDontCareOp(Resources.ScatteringLUT->GetFormat());
	Pass.RenderPass = RPDesc.Construct(Scope);

	std::vector framebufferAttachments =
	{
		GVkImage::ToView(Images.DeltaSR,		  VK_IMAGE_VIEW_TYPE_2D_ARRAY),
		GVkImage::ToView(Images.DeltaSM,		  VK_IMAGE_VIEW_TYPE_2D_ARRAY),
		GVkImage::ToView(Resources.ScatteringLUT, VK_IMAGE_VIEW_TYPE_2D_ARRAY)
	};

	Pass.Framebuffer = std::make_shared<GVkFramebuffer>(Scope, Pass.RenderPass, framebufferAttachments);

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	Pass.Set = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenLayeredVS);
	PSODesc.GS.AppendCode(GShaderPrecomputeLUT::ScatteringGS);
	PSODesc.PS.AddDefine("_ATMO_USE_KILOMETERS")
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderPrecomputeLUT::LUTCommon)
		.AppendCode(GShaderPrecomputeLUT::SingleScatteringPS);
	PSODesc.AddDescriptorLayout(Pass.Set->GetLayout());
	PSODesc.SetRenderPass(Pass.RenderPass);
	Pass.Pipeline = PSODesc.Construct(Scope);

	return Pass;
}

GAtmospherePrecomputeLUT::GLutPass GAtmospherePrecomputeLUT::_addScatteringMultiEvalStep(GVkSharedResources& Resources)
{
	GLutPass Pass{};
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Images.DeltaJ->GetFormat());
	Pass.RenderPass = RPDesc.Construct(Scope);

	Pass.Framebuffer = std::make_shared<GVkFramebuffer>(Scope, Pass.RenderPass, std::vector{ GVkImage::ToView(Images.DeltaJ, VK_IMAGE_VIEW_TYPE_2D_ARRAY) });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.IrradianceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeltaSR), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeltaSM), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	Pass.Set = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenLayeredVS);
	PSODesc.GS.AppendCode(GShaderPrecomputeLUT::ScatteringGS);
	PSODesc.PS.AddDefine("_ATMO_USE_KILOMETERS")
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderPrecomputeLUT::LUTCommon)
		.AppendCode(GShaderPrecomputeLUT::MulitScatteringEvaluatePS);
	PSODesc.AddPushConstants<int>(VK_SHADER_STAGE_FRAGMENT_BIT);
	PSODesc.AddDescriptorLayout(Pass.Set->GetLayout());
	PSODesc.SetRenderPass(Pass.RenderPass);
	Pass.Pipeline = PSODesc.Construct(Scope);
	Pass.PushConstants.push_back(0);

	return Pass;
}

GAtmospherePrecomputeLUT::GLutPass GAtmospherePrecomputeLUT::_addScatteringMultiAddStep(GVkSharedResources& Resources)
{
	GLutPass Pass{};
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(Resources.ScatteringLUT->GetFormat(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	Pass.RenderPass = RPDesc.Construct(Scope);

	Pass.Framebuffer = std::make_shared<GVkFramebuffer>(Scope, Pass.RenderPass, std::vector{ GVkImage::ToView(Resources.ScatteringLUT, VK_IMAGE_VIEW_TYPE_2D_ARRAY) });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeltaJ), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	Pass.Set = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenLayeredVS);
	PSODesc.GS.AppendCode(GShaderPrecomputeLUT::ScatteringGS);
	PSODesc.PS.AddDefine("_ATMO_USE_KILOMETERS")
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderPrecomputeLUT::LUTCommon)
		.AppendCode(GShaderPrecomputeLUT::MulitScatteringAddPS);
	PSODesc.AddDescriptorLayout(Pass.Set->GetLayout());
	PSODesc.SetRenderPass(Pass.RenderPass);

	PSODesc.AttachmentBlendState(0).blendEnable = VK_TRUE;
	PSODesc.AttachmentBlendState(0).colorBlendOp = VK_BLEND_OP_ADD;
	PSODesc.AttachmentBlendState(0).dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	PSODesc.AttachmentBlendState(0).srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	Pass.Pipeline = PSODesc.Construct(Scope);

	return Pass;
}

GAtmospherePrecomputeLUT::GLutPass GAtmospherePrecomputeLUT::_addIrradianceMultiStep(GVkSharedResources& Resources)
{
	GLutPass Pass{};
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(Resources.IrradianceLUT->GetFormat(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	Pass.RenderPass = RPDesc.Construct(Scope);

	Pass.Framebuffer = std::make_shared<GVkFramebuffer>(Scope, Pass.RenderPass, std::vector{ Resources.IrradianceLUT });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeltaSR), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeltaSM), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	Pass.Set = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AddDefine("_ATMO_USE_KILOMETERS")
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaderPrecomputeLUT::LUTCommon)
		.AppendCode(GShaderPrecomputeLUT::IrradianceMultiStepPS);
	PSODesc.AddDescriptorLayout(Pass.Set->GetLayout());
	PSODesc.SetRenderPass(Pass.RenderPass);

	PSODesc.AttachmentBlendState(0).blendEnable = VK_TRUE;
	PSODesc.AttachmentBlendState(0).colorBlendOp = VK_BLEND_OP_ADD;
	PSODesc.AttachmentBlendState(0).dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	PSODesc.AttachmentBlendState(0).srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	Pass.Pipeline = PSODesc.Construct(Scope);

	return Pass;
}

void GAtmospherePrecomputeLUT::_initImages(GVkSharedResources& Resources)
{
	Resources.TransmittanceLUT = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 256, 64, 1 }, EImageFlags::Sampler | EImageFlags::RenderTarget);
	Resources.ScatteringLUT = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 256, 128, 64 }, EImageFlags::Sampler | EImageFlags::RenderTarget);
	Resources.IrradianceLUT = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 64, 16, 1 }, EImageFlags::Sampler | EImageFlags::RenderTarget);

	Images.DeltaSM = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 256, 128, 64 }, EImageFlags::Sampler | EImageFlags::RenderTarget);
	Images.DeltaSR = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 256, 128, 64 }, EImageFlags::Sampler | EImageFlags::RenderTarget);
	Images.DeltaJ = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{ 256, 128, 64 }, EImageFlags::Sampler | EImageFlags::RenderTarget);
}

GAtmospherePrecomputeLUT::GAtmospherePrecomputeLUT(std::shared_ptr<RenderScope> InScope, GVkSharedResources& Resources)
	: Scope(InScope)
{
	_initImages(Resources);

	m_precomputePasses.push_back(_addTransmittancePass(Resources));
	m_precomputePasses.push_back(_addIrradiancePass(Resources));
	m_precomputePasses.push_back(_addSignleScatteringPass(Resources));

	auto IrradianceStep = _addIrradianceMultiStep(Resources);
	auto ScatteringEvalStep = _addScatteringMultiEvalStep(Resources);
	auto ScatteringAddStep = _addScatteringMultiAddStep(Resources);

	for (uint32_t Order = 0; Order < 3; Order++)
	{
		m_precomputePasses.push_back(IrradianceStep);

		ScatteringEvalStep.PushConstants[0] = Order;
		m_precomputePasses.push_back(ScatteringEvalStep);
		m_precomputePasses.push_back(ScatteringAddStep);
	}
}

void GAtmospherePrecomputeLUT::_executePass(std::shared_ptr<GVkCommandBuffer> CommandBuffer, GLutPass& pass)
{
	CommandBuffer->BeginRenderPass(pass.RenderPass, pass.Framebuffer);

	CommandBuffer->BindPipeline(pass.Pipeline);
	CommandBuffer->BindDescriptorSet(0, pass.Set);

	uint32_t constantIndex = 0u;
	for (auto& Constant : pass.PushConstants)
	{
		CommandBuffer->PushConstants(constantIndex, Constant);
		constantIndex++;
	}

	CommandBuffer->Draw(3, pass.Framebuffer->GetExtents().depth);

	CommandBuffer->EndRenderPass();
}

void GAtmospherePrecomputeLUT::_runPrecompute()
{
	std::shared_ptr<GVkCommandBuffer> CommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT);
	for (auto& pass : m_precomputePasses)
		_executePass(CommandBuffer, pass);
}

void GAtmospherePrecomputeLUT::Execute(std::shared_ptr<RenderScope> Scope, GVkSharedResources& Resources)
{
	GAtmospherePrecomputeLUT AtmosphereGen(Scope, Resources);
	AtmosphereGen._runPrecompute();
}