#pragma once

#include <stdexcept>
#include <vector>
#include <string>

#define CHECK_VKRESULT(RESULT, MESSAGE)                                             \
if(RESULT != VK_SUCCESS)                                                            \
{                                                                                   \
	throw std::runtime_error(MESSAGE" (VkResult: " + std::to_string(RESULT) + ")"); \
}

std::vector<char> ReadFile(const std::string& filename);
