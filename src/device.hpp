#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace vkpg
{
class VulkanSwapChain;

class VulkanDevice
{
public:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family;
		// TODO: check if "present" is suitable name
		std::optional<uint32_t> present_family;
		// TODO: add "compute" or "transfer" field

		bool IsComplete()
		{
			return graphics_family.has_value() && present_family.has_value();
		}
	} queue_family_indices;

	VkPhysicalDevice physical_device;
	VkDevice logical_device;

	const VkInstance& instance;
	vkpg::VulkanSwapChain& swap_chain;
	VkSurfaceKHR& surface;

	// TODO: initialize device_extensions in constructor
	const std::vector<const char*> device_extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VulkanDevice(const VkInstance& instance, vkpg::VulkanSwapChain& swap_chain, VkSurfaceKHR& surface);
	~VulkanDevice();

	void CreateLogicalDevice();
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	void PickPhysicalDevice();
	uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
					  VkBuffer& buffer, VkDeviceMemory& buffer_memory);

	VkSampleCountFlagBits GetMaxUsableSampleCount();
};
}
