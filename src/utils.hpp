#pragma once

#include <stdexcept>
#include <vector>
#include <string>

#define CHECK_VKRESULT(RESULT, MESSAGE)                                                                                                     \
if(RESULT != VK_SUCCESS)                                                                                                                    \
{                                                                                                                                           \
	throw std::runtime_error(MESSAGE" (VkResult: " + std::to_string(RESULT) + ") in " + __FILE__ + " at line " + std::to_string(__LINE__)); \
}

std::vector<char> ReadFile(const std::string& filename);
