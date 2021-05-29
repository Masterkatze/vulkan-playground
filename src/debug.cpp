#include "debug.hpp"
#include "utils.hpp"

#include <iostream>
#include <cstring>

const std::vector<const char*> vkpg::Debug::validation_layers =
{
	"VK_LAYER_KHRONOS_validation"
};

VkDebugUtilsMessengerEXT debug_messenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
													VkDebugUtilsMessageTypeFlagsEXT message_type,
													const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
													void *user_data)
{
	#pragma unused(message_severity)
	#pragma unused(message_type)
	#pragma unused(user_data)

	std::cerr << "Validation layer: " << callback_data->pMessage << std::endl;

	return VK_FALSE;
}

void vkpg::Debug::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = DebugCallback;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *create_info,
									  const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *debug_messenger)
{
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	return func ? func(instance, create_info, allocator, debug_messenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *allocator)
{
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if(func != nullptr)
	{
		func(instance, debug_messenger, allocator);
	}
}

void vkpg::Debug::SetupDebugging(VkInstance instance)
{
	if(!enable_validation_layers) return;

	if(!CheckValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	PopulateDebugMessengerCreateInfo(create_info);

	auto result = CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger);
	CHECK_VKRESULT(result, "Failed to set up debug messenger");
}

void vkpg::Debug::TearDownDebugging(VkInstance instance)
{
	if(enable_validation_layers)
	{
		DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
	}
}

bool vkpg::Debug::CheckValidationLayerSupport()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> availableLayers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, availableLayers.data());

	return std::all_of(validation_layers.begin(), validation_layers.end(), [&availableLayers](auto name){
		return std::find_if(availableLayers.begin(), availableLayers.end(), [&name](auto lhs){
			return std::strcmp(name, lhs.layerName) == 0;
		}) != availableLayers.end();
	});
}

const std::vector<const char*>& vkpg::Debug::GetValidationLayers()
{
	return validation_layers;
}
