#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vkpg
{
struct Debug
{
	#ifdef NDEBUG
	static const bool enable_validation_layers = false;
	#else
	static const bool enable_validation_layers = true;
	#endif

	static void SetupDebugging(VkInstance instance);
	static void TearDownDebugging(VkInstance instance);

	static bool CheckValidationLayerSupport();
	static const std::vector<const char*>& GetValidationLayers();

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

private:
	static const std::vector<const char*> validation_layers;
};
}
