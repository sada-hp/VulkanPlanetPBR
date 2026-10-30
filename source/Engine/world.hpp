#pragma once
#include "Components/drawable.hpp"
#include "actor.hpp"

#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include <vector>

class IWorld
{
private:
	std::shared_ptr<GVkDescriptorSet> _transformsSet = VK_NULL_HANDLE;
	std::shared_ptr<GVkDescriptorSet> _indirectSet = VK_NULL_HANDLE;
	mutable bool bDescriptorsDirty = false;

protected:
	entt::registry registry;

protected:
	glm::vec3 sunDirection = glm::normalize(glm::vec3(1.0));
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

private:
	void register_gpu_actor(entt::registry& reg, entt::entity ent);
	void remove_gpu_actor(entt::registry& reg, entt::entity ent);
	void update_gpu_actor(entt::registry& reg, entt::entity ent);
	
	void update_descriptors();

public:
	IWorld(std::shared_ptr<RenderScope> Scope);
	virtual ~IWorld();

	virtual const glm::vec3& GetSunDirection() const { return sunDirection; }
	virtual void SetSunDirection(const glm::vec3& newDir) { sunDirection = glm::normalize(newDir); }

	const entt::registry& GetRegistry() const { return registry; }
	std::shared_ptr<GActor> NewActor();

	std::shared_ptr<GVkDescriptorSet> GetSceneTransformsSet() const;
	std::shared_ptr<GVkDescriptorSet> GetSceneIndirectSet() const;
};