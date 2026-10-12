#pragma once
#include "Components/drawable.hpp"
#include "glm/glm.hpp"
#include <vector>

class IWorld
{
protected:
	glm::vec3 sunDirection = glm::normalize(glm::vec3(1.0));

public:
	virtual void GetDrawableObjects(std::vector<std::pair<const GDrawable*, const EllipsoidMatrix*>>&) const = 0;
	virtual void SetSunDirection(const glm::vec3& newDir) { sunDirection = glm::normalize(newDir); }
	virtual const glm::vec3& GetSunDirection() const { return sunDirection; }
};