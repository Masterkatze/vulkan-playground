#include "events.hpp"

#include <GLFW/glfw3.h>

void vkpg::Events::KeyCallback(void* window, int key, int scancode, int action, int mods)
{
	//auto glfw_window = static_cast<GLFWwindow*>(window_);

	//std::cout << "key=" << key << " scancode=" << scancode << " action=" << action << " mods" << mods << std::endl;

	if(key == GLFW_KEY_LEFT  && (action == GLFW_PRESS || action == GLFW_REPEAT)) object_rotation.x--;
	if(key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) object_rotation.x++;
	if(key == GLFW_KEY_DOWN  && (action == GLFW_PRESS || action == GLFW_REPEAT)) object_rotation.y--;
	if(key == GLFW_KEY_UP    && (action == GLFW_PRESS || action == GLFW_REPEAT)) object_rotation.y++;

	if(key == GLFW_KEY_S)
	{
		if(action == GLFW_PRESS) camera.keys.up = true;
		if(action == GLFW_RELEASE) camera.keys.up = false;
	}
	if(key == GLFW_KEY_W)
	{
		if(action == GLFW_PRESS) camera.keys.down = true;
		if(action == GLFW_RELEASE) camera.keys.down = false;
	}
	if(key == GLFW_KEY_D)
	{
		if(action == GLFW_PRESS) camera.keys.left = true;
		if(action == GLFW_RELEASE) camera.keys.left = false;
	}
	if(key == GLFW_KEY_A)
	{
		if(action == GLFW_PRESS) camera.keys.right = true;
		if(action == GLFW_RELEASE) camera.keys.right = false;
	}
}

void vkpg::Events::MouseButtonCallback(void* window, int button, int action, int mods)
{
	//std::cout << "button=" << button << " action=" << action << " mods" << mods << std::endl;

	if(button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if(action == GLFW_PRESS) mouse_buttons.left = true;
		if(action == GLFW_RELEASE) mouse_buttons.left = false;
	}
	if(button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if(action == GLFW_PRESS) mouse_buttons.right = true;
		if(action == GLFW_RELEASE) mouse_buttons.right = false;
	}
	if(button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if(action == GLFW_PRESS) mouse_buttons.middle = true;
		if(action == GLFW_RELEASE) mouse_buttons.middle = false;
	}
}

void vkpg::Events::CursorPositionCallback(void* window, double x, double y)
{

	auto dx = mouse_pos.x - x;
	auto dy = mouse_pos.y - y;

	if (mouse_buttons.left)
	{
		camera.Rotate(glm::vec3(dy * camera.rotation_speed, -dx * camera.rotation_speed, 0.0f));
	}
	if (mouse_buttons.right)
	{
		camera.Translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
	}
	if (mouse_buttons.middle)
	{
		camera.Translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));
	}

	mouse_pos = glm::vec2(x, y);
}
