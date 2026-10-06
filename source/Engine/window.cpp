#include "pch.hpp"
#include "window.hpp"
#include "event_listener.hpp"

void GWindow::glfw_key_press(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	EventContext* context = static_cast<EventContext*>(glfwGetWindowUserPointer(window));
	context->evnt->Register(GEvents::KeyPress((GEnums::EKey)key, (GEnums::EAction)action));
}

void GWindow::glfw_mouse_press(GLFWwindow* window, int button, int action, int mods)
{
	EventContext* context = static_cast<EventContext*>(glfwGetWindowUserPointer(window));
	context->evnt->Register(GEvents::MousePress((GEnums::EMouse)button, (GEnums::EAction)action));
}

void GWindow::glfw_mouse_move(GLFWwindow* window, double xpos, double ypos)
{
	static double oldx = xpos;
	static double oldy = ypos;

	EventContext* context = static_cast<EventContext*>(glfwGetWindowUserPointer(window));
	context->evnt->Register(GEvents::MousePosition(xpos, ypos, xpos - oldx, ypos - oldy));

	oldx = xpos;
	oldy = ypos;
}

void GWindow::glfw_scroll(GLFWwindow* window, double dx, double dy)
{
	EventContext* context = static_cast<EventContext*>(glfwGetWindowUserPointer(window));
	context->evnt->Register(GEvents::ScrollDelta(dx, dy));
}

GWindow::GWindow(int width, int height, const char* title)
{
	glfwInit();
	assert(glfwVulkanSupported());
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	m_GlfwWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);

	m_WindowPointer.ctx = this;
	glfwSetWindowUserPointer(m_GlfwWindow, &m_WindowPointer);

	m_Renderer = new GVulkanBase(m_GlfwWindow);
}

GWindow::~GWindow()
{
	if (m_GlfwWindow)
	{
		delete m_Renderer;
		glfwDestroyWindow(m_GlfwWindow);
		glfwTerminate();

		m_GlfwWindow = nullptr;
		m_Renderer = nullptr;
	}
}

void GWindow::SetUpEvents(GEventListener& listener)
{
	m_WindowPointer.evnt = &listener;
	glfwSetKeyCallback(m_GlfwWindow, glfw_key_press);
	glfwSetMouseButtonCallback(m_GlfwWindow, glfw_mouse_press);
	glfwSetCursorPosCallback(m_GlfwWindow, glfw_mouse_move);
	glfwSetScrollCallback(m_GlfwWindow, glfw_scroll);
}

void GWindow::SetTitle(const char* title)
{
	glfwSetWindowTitle(m_GlfwWindow, title);
}

void GWindow::SetWindowSize(int width, int height)
{
	glfwSetWindowSize(m_GlfwWindow, width, height);
}

void GWindow::MinimizeWindow()
{
	glfwIconifyWindow(m_GlfwWindow);
}

glm::ivec2 GWindow::GetWindowSize() const
{
	int width = 0, height = 0;
	glfwGetWindowSize(m_GlfwWindow, &width, &height);
	return glm::ivec2{ width, height };
}

glm::vec2 GWindow::GetCursorPos() const
{
	double xpos = 0.0, ypos = 0.0;
	glfwGetCursorPos(m_GlfwWindow, &xpos, &ypos);
	return glm::vec2{ xpos, ypos };
}

double GWindow::GetAspectRatio() const
{
	int width = 0, height = 0;
	glfwGetWindowSize(m_GlfwWindow, &width, &height);

	return double(width) / double(height);
}

void GWindow::SetCursorPos(double xpos, double ypos)
{
	glfwSetCursorPos(m_GlfwWindow, xpos, ypos);
}

void GWindow::ShowCursor(bool bShow)
{
	glfwSetInputMode(m_GlfwWindow, GLFW_CURSOR, bShow ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

void GWindow::DisableCursor(bool bDisabled)
{
	glfwSetInputMode(m_GlfwWindow, GLFW_CURSOR, bDisabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void GWindow::SetAttribute(int attrib, int value)
{
	glfwSetWindowAttrib(m_GlfwWindow, attrib, value);
}

GVulkanBase& GWindow::GetRenderer() const
{
	return *m_Renderer;
}

bool GWindow::IsAlive() const
{
	return !glfwWindowShouldClose(m_GlfwWindow);
}

void GWindow::ProcessEvents() const
{
	glfwPollEvents();
}