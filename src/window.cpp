#include "window.hpp"
#include "utils.hpp"
#include "swapchain.hpp"
#include "debug.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::function<void(void*, int, int)> vkpg::VulkanWindow::framebuffer_resize_callback;

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

void vkpg::VulkanWindow::GetFramebufferSize(int& width, int& height)
{
	glfwGetFramebufferSize(window, &width, &height);
}

void vkpg::VulkanWindow::SetFramebufferResizeCallback(std::function<void(void*, int, int)> callback)
{
	framebuffer_resize_callback = callback;
}

void vkpg::VulkanWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	framebuffer_resize_callback(reinterpret_cast<void*>(glfwGetWindowUserPointer(window)), width, height);
}

std::vector<const char*> vkpg::VulkanWindow::GetRequiredExtensions()
{
	uint32_t glfw_extension_count = 0;
	const char **glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if(vkpg::Debug::enable_validation_layers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}
