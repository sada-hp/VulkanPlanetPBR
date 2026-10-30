#pragma once
#include "Engine/object.hpp"

struct IComponent : public GObject
{
	virtual ~IComponent() {}
};