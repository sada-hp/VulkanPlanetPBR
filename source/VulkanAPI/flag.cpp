#include "flag.hpp"

GVkFlag::GVkFlag(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{
	_internal.resize(Scope->GetMaxFramesInFlight(), true);
}

bool GVkFlag::IsSet() const
{
	return _internal[Scope->GetResourceIndex()];
}

void GVkFlag::MarkClear()
{
	_internal[Scope->GetResourceIndex()] = false;
}

void GVkFlag::MarkDirty()
{
	for (uint32_t i = 0; i < _internal.size(); i++)
		_internal[i] = true;
}