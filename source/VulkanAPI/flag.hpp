#pragma once
#include "VulkanAPI/scope.hpp"

class GVkFlag : public IVkObj
{
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<bool> _internal;

public:
	GVkFlag(std::shared_ptr<RenderScope> InScope);
	bool IsSet() const;
	void MarkClear();
	void MarkDirty();
};