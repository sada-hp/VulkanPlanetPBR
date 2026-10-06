#include "application.hpp"
#include "window.hpp"

IGApplication::IGApplication(const SWindowParameters& Parameters)
	: Window(Parameters.Width, Parameters.Height, Parameters.Title.c_str())
{
}

IGApplication::~IGApplication()
{
}

void IGApplication::Run()
{
	_timestamp = glfwGetTime();

	while (Window.IsAlive())
	{
		double time = glfwGetTime();
		double delta = time - _timestamp;

		Window.ProcessEvents();
		Update(delta);

		auto& Renderer = Window.GetRenderer();
		Renderer.Draw(Camera, GetWorld());

		_timestamp = time;
	}
}