#include "device.hpp"
#include "debug.hpp"
#include "utils.hpp"
#include "swapchain.hpp"

#include <set>
#include <iostream>

vkpg::VulkanDevice::VulkanDevice(const VkInstance& instance, VulkanSwapChain& swap_chain, VkSurfaceKHR& surface) :
    instance(instance), swap_chain(swap_chain), surface(surface)
{

}

void vkpg::VulkanDevice::Cleanup()
{
	vkDestroyDevice(logical_device, nullptr);
}

void vkpg::VulkanDevice::CreateLogicalDevice()
{
	auto queue_family_indices = FindQueueFamilies(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families =
	{
		queue_family_indices.graphics_family.value(),
		queue_family_indices.present_family.value()
	};

	float queue_priority = 1.0f;
	for(auto queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queue_family;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queue_priority;
		queue_create_infos.emplace_back(std::move(queueCreateInfo));
	}

	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());

	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if(vkpg::Debug::enable_validation_layers)
	{
		const auto& validation_layers = vkpg::Debug::GetValidationLayers();
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	auto result = vkCreateDevice(physical_device, &create_info, nullptr, &logical_device);
	CHECK_VKRESULT(result, "Failed to create logical device");

	vkGetDeviceQueue(logical_device, queue_family_indices.graphics_family.value(), 0, &swap_chain.graphics_queue);
	vkGetDeviceQueue(logical_device, queue_family_indices.present_family.value(), 0, &swap_chain.present_queue);
}

vkpg::VulkanDevice::QueueFamilyIndices vkpg::VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
{
	QueueFamilyIndices queue_family_indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for(const auto& queue_family : queue_families)
	{
		if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queue_family_indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

		if(present_support)
		{
			queue_family_indices.present_family = i;
		}

		if(queue_family_indices.IsComplete())
		{
			break;
		}

		i++;
	}

	return queue_family_indices;
}

bool vkpg::VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices queue_family_indices = FindQueueFamilies(device);
	bool extensions_supported = CheckDeviceExtensionSupport(device);

	bool swap_chain_adequate = false;
	if(extensions_supported)
	{
		auto swap_chain_support = swap_chain.QuerySwapChainSupport(device);
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return queue_family_indices.IsComplete() &&
	       extensions_supported &&
	       swap_chain_adequate &&
	       supported_features.samplerAnisotropy;
}

bool vkpg::VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extension_count;
	auto result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
	CHECK_VKRESULT(result, "Failed to enumerate device extension properties");

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
	CHECK_VKRESULT(result, "Failed to enumerate device extension properties");

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for(const auto& extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

void vkpg::VulkanDevice::PickPhysicalDevice()
{
	uint32_t device_count = 0;
	auto result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	CHECK_VKRESULT(result, "Failed to enumerate physical devices");

	if(device_count == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> physical_devices(device_count);
	result = vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
	CHECK_VKRESULT(result, "Failed to enumerate physucal devices");

	auto it = std::find_if(physical_devices.begin(), physical_devices.end(), [this](const auto& d)
	{
		return IsDeviceSuitable(d);
	});

	if(it == physical_devices.end())
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
	physical_device = *it;

	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

	std::cout << "Physical device: " << physical_device_properties.deviceName << std::endl;
}

uint32_t vkpg::VulkanDevice::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for(uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void vkpg::VulkanDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                      VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	auto result = vkCreateBuffer(logical_device, &buffer_info, nullptr, &buffer);
	CHECK_VKRESULT(result, "Failed to create vertex buffer");

	VkMemoryRequirements mempry_requirements;
	vkGetBufferMemoryRequirements(logical_device, buffer, &mempry_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mempry_requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(mempry_requirements.memoryTypeBits, properties);

	result = vkAllocateMemory(logical_device, &alloc_info, nullptr, &buffer_memory);
	CHECK_VKRESULT(result, "Failed to allocate vertex buffer memory");

	vkBindBufferMemory(logical_device, buffer, buffer_memory, 0);
}

VkSampleCountFlagBits vkpg::VulkanDevice::GetMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

	VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts &
	                            physical_device_properties.limits.framebufferDepthSampleCounts;

	if(counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if(counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if(counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if(counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT; }
	if(counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT; }
	if(counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}
