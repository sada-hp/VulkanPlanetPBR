#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "IRenderStage.hpp"

class GAttachmentsClear : public IRenderStage
{
protected:
	void setup_dependencies(const IWorld& World) override;
	void record(const GCamera& Camera, const IWorld& World) override;

public:
	GAttachmentsClear(std::shared_ptr<RenderScope> Scope);
	virtual ~GAttachmentsClear();
};