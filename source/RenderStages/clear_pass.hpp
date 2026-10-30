#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "IRenderStage.hpp"

class GAttachmentsClear : public IRenderStage
{
public:
	GAttachmentsClear(std::shared_ptr<RenderScope> Scope);
	virtual ~GAttachmentsClear();
	void Execute(const GCamera& Camera, const IWorld& World) override;
};