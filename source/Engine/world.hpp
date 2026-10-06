#pragma once
#include "Components/drawable.hpp"
#include <vector>

class IWorld
{
public:
	virtual const std::vector<GDrawable>& GetDrawableObjects() const = 0;
	virtual void Clear() = 0;
};