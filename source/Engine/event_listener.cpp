#include "pch.hpp"
#include "event_listener.hpp"

void GEventListener::SetUserPointer(void* pointer)
{
	m_UserPointer = pointer;
}

void GEventListener::Register(GEvents::KeyPress e) const
{
	std::for_each(m_keyPressEvents.begin(), m_keyPressEvents.end(), [&](const std::function<void(GEvents::KeyPress, void*)>& f)
	{
		f(e, m_UserPointer);
	});
}

void GEventListener::Register(GEvents::MousePress e) const
{
	std::for_each(m_mousePressEvents.begin(), m_mousePressEvents.end(), [&](const std::function<void(GEvents::MousePress, void*)>& f)
	{
		f(e, m_UserPointer);
	});
}

void GEventListener::Register(GEvents::MousePosition e) const
{
	std::for_each(m_mouseMoveEvents.begin(), m_mouseMoveEvents.end(), [&](const std::function<void(GEvents::MousePosition, void*)>& f)
	{
		f(e, m_UserPointer);
	});
}

void GEventListener::Register(GEvents::ScrollDelta e) const
{
	std::for_each(m_scrollEvents.begin(), m_scrollEvents.end(), [&](const std::function<void(GEvents::ScrollDelta, void*)>& f)
	{
		f(e, m_UserPointer);
	});
}