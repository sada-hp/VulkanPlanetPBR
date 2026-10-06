#pragma once
#include "core.hpp"
#include "window.hpp"
#include "world.hpp"

struct SWindowParameters
{
	int Width = 1024;
	int Height = 720;
	std::string Title = "Window";
};

class IGApplication
{
private:
	double _timestamp = 0.0;

public:
	GWindow Window;
	GCamera Camera;

protected:
	virtual void Update(float Delta) = 0;

public:
	IGApplication(const SWindowParameters& Parameters);
	virtual ~IGApplication();
	void Run();

	virtual const IWorld& GetWorld() const = 0;
};