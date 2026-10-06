#pragma once
#include "core.hpp"
#include "glm/glm.hpp"
#include "Engine/enums.hpp"

namespace GEvents
{
	struct MousePosition { double x; double y; double delta_x; double delta_y; };

	struct ScrollDelta { double x; double y; };

	struct MousePress { GEnums::EMouse key; GEnums::EAction action; };

	struct KeyPress { GEnums::EKey key; GEnums::EAction action; };
}

class GEventListener
{
	friend class GWindow;

private:
	std::vector<std::function<void(GEvents::MousePosition, void*)>> m_mouseMoveEvents = {};
	std::vector<std::function<void(GEvents::MousePress, void*)>> m_mousePressEvents = {};
	std::vector<std::function<void(GEvents::ScrollDelta, void*)>> m_scrollEvents = {};
	std::vector<std::function<void(GEvents::KeyPress, void*)>> m_keyPressEvents = {};
	void* m_UserPointer = nullptr;

protected:
	void Register(GEvents::KeyPress) const;
	void Register(GEvents::MousePress) const;
	void Register(GEvents::ScrollDelta) const;
	void Register(GEvents::MousePosition) const;

public:
	GEventListener()
	{
	}

	~GEventListener() = default;
	void SetUserPointer(void* pointer);

public:
	template<typename T, typename Callback = void(T::*)(GEvents::KeyPress, void*)>
	inline void SubscribeKeyPressEvent(T* object, Callback func)
	{
		std::function lambda = [object, func](GEvents::KeyPress e, void* p) {
			std::invoke(func, object, e, p);
		};

		m_keyPressEvents.push_back(lambda);
	}

	template<typename T, typename Callback = void(T::*)(GEvents::MousePress, void*)>
	inline void SubscribeMousePressEvent(T* object, Callback func)
	{
		std::function lambda = [object, func](GEvents::MousePress e, void* p) {
			std::invoke(func, object, e, p);
		};

		m_mousePressEvents.push_back(lambda);
	}

	template<typename T, typename Callback = void(T::*)(GEvents::ScrollDelta, void*)>
	inline void SubscribeScrollEvent(T* object, Callback func)
	{
		std::function lambda = [object, func](GEvents::ScrollDelta e, void* p) {
			std::invoke(func, object, e, p);
		};

		m_scrollEvents.push_back(lambda);
	}

	template<typename T, typename Callback = void(T::*)(GEvents::MousePosition, void*)>
	inline void SubscribeMouseMoveEvent(T* object, Callback func)
	{
		std::function lambda = [object, func](GEvents::MousePosition e, void* p) {
			std::invoke(func, object, e, p);
		};

		m_mouseMoveEvents.push_back(lambda);
	}
};