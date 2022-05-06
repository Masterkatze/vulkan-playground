#pragma once

#include <vulkan/vulkan_core.h>

#include <experimental/source_location>
#include <stdexcept>
#include <string>
#include <vector>

inline void CheckVkResult(VkResult result, const std::string& message, const std::experimental::source_location& location = std::experimental::source_location::current())
{
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error(message + " (VkResult=" + std::to_string(result) + ") in " + location.function_name() + " (" + location.file_name() + ":" + std::to_string(location.line()) + ")");
	}
}

std::vector<char> ReadFile(const std::string& filename);
