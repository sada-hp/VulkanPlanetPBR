#pragma once
#include "VulkanAPI/renderer.hpp"
#include "glfw/glfw3.h"
#include "glm/glm.hpp"
#include "core.hpp"

class GEventListener;
class GWindow;

typedef void(*GameLoopFunc)(float, const GWindow&, const GVulkanBase&);

class GWindow
{
	friend class IGApplication;

private:
	GLFWwindow* m_GlfwWindow = nullptr;
	GVulkanBase* m_Renderer = nullptr;

	struct EventContext
	{
		GWindow* ctx;
		GEventListener* evnt;
	} m_WindowPointer;

private:
	static void glfw_key_press(GLFWwindow* window, int, int, int, int);

	static void glfw_mouse_press(GLFWwindow* window, int, int, int);

	static void glfw_mouse_move(GLFWwindow* window, double, double);

	static void glfw_scroll(GLFWwindow* window, double, double);

protected:
	GWindow(int width, int height, const char* title);
	~GWindow();

public:
	/*
	* !@brief Connect this window to in-engine event listener
	*/
	void SetUpEvents(GEventListener& listener);
	/*
	* !@brief Sets the title of the window
	* 
	* @param[in] title - new title string
	*/
	void SetTitle(const char* title);
	/*
	* !@brief Set the size of the window
	* 
	* @param[in] width - new width of the window
	* @param[in] height - new height of the window
	*/
	void SetWindowSize(int width, int height);
	/*
	* !@brief Minimizes (iconifies) the window
	*/
	void MinimizeWindow();
	/*
	* !@brief Get the size of the window
	* 
	* @return Vector containing integer width, height of the window
	*/
	glm::ivec2 GetWindowSize() const;
	/*
	* !@brief Get the curent position of the cursor, relative to window
	* 
	* @return Vector containing x, y coordinate of cursor
	*/
	glm::vec2 GetCursorPos() const;
	/*
	* !@brief Get aspect ration of the window
	* 
	* @return Double representing the ratio between width and height of the window
	*/
	double GetAspectRatio() const;
	/*
	* !@brief Set cursor position on the window
	* 
	* @param[in] xpos - new x coordinate of the cursor
	* @param[in] ypos - new y coordinate of the cursor
	*/
	void SetCursorPos(double xpos, double ypos);
	/*
	* !@brief Hides/Shows cursor when it hovers the window
	* 
	* @param[in] bShow - new state of the cursor
	*/
	void ShowCursor(bool bShow);
	/*
	* !@brief Hides/Shows and locks/unlocks the cursor
	* 
	* @param[in] bDisabled - new state of the cursor
	*/
	void DisableCursor(bool bDisabled);
	/*
	* !@brief Sets other GLFW attributes for the window
	* 
	* @param[in] attrib - GLFW attribute
	* @param[in] value - value of the attribute
	*/
	void SetAttribute(int attrib, int value);
	/*
	* !@brief Get renderer interface reference
	*/
	GVulkanBase& GetRenderer() const;
	/*
	* !@brief Check if window was queued for closure
	*/
	bool IsAlive() const;
	/*
	* !@brief Process window event queue
	*/
	void ProcessEvents() const;
};
