#include "window.hpp"

#include "debug.hpp"
#include "swapchain.hpp"
#include "utils.hpp"
#include <iostream>

vkpg::VulkanWindow::FramebufferResizeCallbackFunction vkpg::VulkanWindow::framebuffer_resize_callback;
vkpg::VulkanWindow::KeyCallbackFunction vkpg::VulkanWindow::key_callback;
vkpg::VulkanWindow::MouseButtonCallbackFunction vkpg::VulkanWindow::mouse_button_callback;
vkpg::VulkanWindow::CursorPositionCallbackFunction vkpg::VulkanWindow::cursor_position_callback;

vkpg::VulkanWindow::VulkanWindow(VulkanSwapChain& swap_chain, VkSurfaceKHR& surface, const VkInstance& instance) : window(nullptr), swap_chain(swap_chain), surface(surface), instance(instance)
{
}

void error_callback(int code, const char* description)
{
    std::cout << "code=" << code << " description=" << description << std::endl;
}

void vkpg::VulkanWindow::Init()
{
	if(glfwInit() == GLFW_FALSE)
	{
		Error("Failed to init glfw");
	}

	glfwSetErrorCallback(error_callback);

	if(glfwVulkanSupported() == GLFW_FALSE)
	{
		Error("Vulkan is not supported");
	}

	// Don't create an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, "Vulkan Playground", nullptr, nullptr);
	if(window == nullptr)
	{
		glfwError("Failed to create glfw window");
	}

	glfwSetWindowUserPointer(window, this);

	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPositionCallback);

	glfwSetWindowSizeLimits(window, 256, 256, INT32_MAX, INT32_MAX);

	if(glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
}

void vkpg::VulkanWindow::CreateSurface()
{
	auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if(result != VK_SUCCESS)
	{
		// TODO: print result too
		glfwError("Failed to create glfw window surface");
	}
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

void vkpg::VulkanWindow::GetFramebufferSize(int& width, int& height) const
{
	glfwGetFramebufferSize(window, &width, &height);
}

void vkpg::VulkanWindow::SetFramebufferResizeCallback(std::function<void(void*, int, int)> callback)
{
	framebuffer_resize_callback = std::move(callback);
}

void vkpg::VulkanWindow::SetKeyCallback(std::function<void(void*, int, int, int, int)> callback)
{
	key_callback = std::move(callback);
}

void vkpg::VulkanWindow::SetMouseButtonCallback(std::function<void(void*, int, int, int)> callback)
{
	mouse_button_callback = std::move(callback);
}

void vkpg::VulkanWindow::SetCursorPositionCallback(std::function<void(void*, int, int)> callback)
{
	cursor_position_callback = std::move(callback);
}

void vkpg::VulkanWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	framebuffer_resize_callback(reinterpret_cast<void*>(glfwGetWindowUserPointer(window)), width, height);
}

void vkpg::VulkanWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	key_callback(reinterpret_cast<void*>(glfwGetWindowUserPointer(window)), key, scancode, action, mods);
}

void vkpg::VulkanWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if(action == GLFW_PRESS)   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if(action == GLFW_RELEASE) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	mouse_button_callback(reinterpret_cast<void*>(glfwGetWindowUserPointer(window)), button, action, mods);
}

void vkpg::VulkanWindow::CursorPositionCallback(GLFWwindow* window, double x, double y)
{
	cursor_position_callback(reinterpret_cast<void*>(glfwGetWindowUserPointer(window)), x, y);
}

std::vector<const char*> vkpg::VulkanWindow::GetRequiredExtensions()
{
	uint32_t glfw_extension_count = 0;
	auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	if(glfw_extensions == nullptr)
	{
		// TODO: print result too
		glfwError("Failed to get glfw required instance extensions");
	}

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if(vkpg::Debug::enable_validation_layers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

GLFWwindow* vkpg::VulkanWindow::GetNativeHandler()
{
	return window;
}
