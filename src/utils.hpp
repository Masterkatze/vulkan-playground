#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <experimental/source_location>
#include <stdexcept>
#include <string>
#include <vector>

inline void Error(const std::string& message, const std::experimental::source_location& location = std::experimental::source_location::current())
{
	throw std::runtime_error(message + " in " + location.function_name() + " (" + location.file_name() + ":" + std::to_string(location.line()) + ")");
}

inline void CheckVkResult(VkResult result, const std::experimental::source_location& location = std::experimental::source_location::current())
{
	if(result != VK_SUCCESS)
	{
		Error("VkResult=" + std::to_string(result), location);
	}
}

inline void CheckVkResult(VkResult result, const std::string& message, const std::experimental::source_location& location = std::experimental::source_location::current())
{
	if(result != VK_SUCCESS)
	{
		Error(message + " (VkResult=" + std::to_string(result) + ")", location);
	}
}

inline void glfwError(const std::string& message, const std::experimental::source_location& location = std::experimental::source_location::current())
{
	const char* error_description;
	glfwGetError(&error_description);
	Error(message + "(" + error_description + ")", location);
}

std::vector<char> ReadFile(const std::string& filename);
