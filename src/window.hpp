#pragma once

#include <GLFW/glfw3.h>

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
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
	void GetFramebufferSize(int& width, int& height);

	GLFWwindow *window;

private:
	vkpg::VulkanSwapChain& swap_chain;
	VkSurfaceKHR& surface;
	const VkInstance& instance;
};
}
