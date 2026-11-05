#pragma once
#include "IRenderStage.hpp"

class GPreprocessStage : public IRenderStage
{
	std::shared_ptr<GVkBuffer> _stagingBuffer = VK_NULL_HANDLE;

protected:
	std::shared_ptr<GVkBufferView> _stagingview = VK_NULL_HANDLE;
	std::shared_ptr<GVkBufferView> _uboView = VK_NULL_HANDLE;

protected:
	void setup_dependencies(const IWorld& World) override;
	void record(const GCamera& Camera, const IWorld& World) override;

public:
	GPreprocessStage(std::shared_ptr<RenderScope> Scope);
	virtual ~GPreprocessStage();
};