#pragma once

#include <GLFW/glfw3.h>

#include <functional>

namespace vkpg
{

class VulkanSwapChain;

class VulkanWindow
{
public:
	using FramebufferResizeCallbackFunction = std::function<void(void*, int, int)>;
	using KeyCallbackFunction = std::function<void(void*, int, int, int, int)>;
	using MouseButtonCallbackFunction = std::function<void(void*, int, int, int)>;
	using CursorPositionCallbackFunction = std::function<void(void*, int, int)>;

	VulkanWindow(vkpg::VulkanSwapChain& swap_chain, VkSurfaceKHR& surface, const VkInstance& instance);
	void Init();
	void CreateSurface();
	void Cleanup();

	bool ShouldClose();
	void PollEvents();

	void GetFramebufferSize(int& width, int& height) const;

	static void SetFramebufferResizeCallback(FramebufferResizeCallbackFunction callback);
	static void SetKeyCallback(KeyCallbackFunction callback);
	static void SetMouseButtonCallback(std::function<void(void*, int, int, int)> callback);
	static void SetCursorPositionCallback(std::function<void(void*, int, int)> callback);

	static std::vector<const char*> GetRequiredExtensions();

	GLFWwindow* GetNativeHandler();

private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void CursorPositionCallback(GLFWwindow* window, double x, double y);

	GLFWwindow *window;

	vkpg::VulkanSwapChain& swap_chain;
	VkSurfaceKHR& surface;
	const VkInstance& instance;

	static FramebufferResizeCallbackFunction framebuffer_resize_callback;
	static KeyCallbackFunction key_callback;
	static MouseButtonCallbackFunction mouse_button_callback;
	static CursorPositionCallbackFunction cursor_position_callback;

	uint32_t width = 800;
	uint32_t height = 600;
};

} // namespace vkpg
