#pragma once

#include <GLFW/glfw3.h>

#include <functional>

namespace vkpg
{
class VulkanSwapChain;

class VulkanWindow
{
public:
	VulkanWindow(vkpg::VulkanSwapChain& swap_chain, VkSurfaceKHR& surface, const VkInstance& instance);
	void Init();
	void CreateSurface();
	void Cleanup();

	bool ShouldClose();
	void PollEvents();

	void GetFramebufferSize(int& width, int& height);
	void SetFramebufferResizeCallback(std::function<void(void*, int, int)> callback);
	static std::vector<const char*> GetRequiredExtensions();

	GLFWwindow *window;

private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	vkpg::VulkanSwapChain& swap_chain;
	VkSurfaceKHR& surface;
	const VkInstance& instance;

	static std::function<void(void*, int, int)> framebuffer_resize_callback;
};
}
