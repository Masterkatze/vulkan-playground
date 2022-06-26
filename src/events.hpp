#pragma once

#include <glm/glm.hpp>

#include "camera.hpp"

namespace vkpg
{

class Events
{
public:
	Events(Camera& camera) : camera(camera) {}

	void KeyCallback(void* window, int key, int scancode, int action, int mods);
	void MouseButtonCallback(void* window, int button, int action, int mods);
	void CursorPositionCallback(void* window, double x, double y);

private:
	Camera& camera;

	glm::vec2 mouse_pos;
	glm::vec3 object_rotation;

	struct
	{
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouse_buttons;
};

} // namespace vkpg
