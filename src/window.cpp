#include "window.hpp"
#include "utils.hpp"
#include "swapchain.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

vkpg::VulkanWindow::VulkanWindow(VulkanSwapChain& swap_chain, VkSurfaceKHR& surface, const VkInstance& instance) : window(nullptr), swap_chain(swap_chain), surface(surface), instance(instance)
{
}

void vkpg::VulkanWindow::Init()
{
	glfwInit();

	// Don't create an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	glfwSetWindowSizeLimits(window, 256, 256, INT32_MAX, INT32_MAX);
}

void vkpg::VulkanWindow::CreateSurface()
{
	auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	CHECK_VKRESULT(result, "Failed to create window surface");
}

void vkpg::VulkanWindow::Cleanup()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

bool vkpg::VulkanWindow::ShouldClose()
{
	return glfwWindowShouldClose(window);
}

void vkpg::VulkanWindow::PollEvents()
{
	glfwPollEvents();
}

void vkpg::VulkanWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	#pragma unused(width)
	#pragma unused(height)

	// TODO: move cast to Application
//	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
//	app->framebuffer_resized = true;
}

void vkpg::VulkanWindow::GetFramebufferSize(int& width, int& height)
{
	glfwGetFramebufferSize(window, &width, &height);
}
