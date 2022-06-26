#pragma once

#include "device.hpp"
#include "window.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>
#include <cstring>

namespace vkpg
{

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
	alignas(16) glm::vec4 camera_position;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texture_coordinates;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription binding_description{};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding_description;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, position);

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, color);

		attribute_descriptions[2].binding = 0;
		attribute_descriptions[2].location = 2;
		attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[2].offset = offsetof(Vertex, texture_coordinates);

		return attribute_descriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return position == other.position && color == other.color && texture_coordinates == other.texture_coordinates;
	}
};

}

namespace std {
template<> struct hash<vkpg::Vertex>
	{
		size_t operator()(vkpg::Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texture_coordinates) << 1);
		}
	};
}

namespace vkpg
{
class VulkanSwapChain
{
private:
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

public:
	VulkanSwapChain(vkpg::VulkanDevice& vulkan_device, vkpg::VulkanWindow& window, VkSurfaceKHR& surface);
	void Create();
	void Cleanup();

	void Recreate();

	void CreateImageViews();
	void CreateRenderPass();
	void CreateUiRenderPass();
	void CreateGraphicsPipeline();
	void CreateColorResources();
	void CreateDepthResources();
	void CreateFramebuffers();
	void CreateUiFramebuffers();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateUiDescriptorPool();
	void CreateDescriptorSets();
	void CreateCommandBuffers();
	void CreateUiCommandBuffers();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateCommandPool();
	void CreateUiCommandPool();
	void CreateDescriptorSetLayout();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, int32_t mip_levels);
	void CreateImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling,
	                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);

	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool pool);
	void EndSingleTimeCommands(VkCommandPool pool, VkCommandBuffer command_buffer);

	void CopyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size);

	void TransitionImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void GenerateMipmaps(VkImage image, VkFormat image_format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels);

	VkSwapchainKHR swap_chain;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> command_buffers;

	VkCommandPool ui_command_pool;
	std::vector<VkCommandBuffer> ui_command_buffers;

	VkQueue graphics_queue;
	VkQueue present_queue;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<VkImage> images;
	VkExtent2D extent;

	std::vector<VkDeviceMemory> uniform_buffers_memory;

	VkDescriptorPool descriptor_pool;
	VkDescriptorPool ui_descriptor_pool;

	VkRenderPass render_pass;
	VkRenderPass ui_render_pass;

	VkPipelineCache pipeline_cache{nullptr};

	uint32_t image_count{};

	VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkFramebuffer> ui_framebuffers;

private:
	vkpg::VulkanDevice& vulkan_device;
	vkpg::VulkanWindow& window;
	VkSurfaceKHR& surface;

	VkFormat image_format;

	std::vector<VkImageView> image_views;
	std::vector<VkFramebuffer> framebuffers;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	VkImage color_image;
	VkDeviceMemory color_image_memory;
	VkImageView color_image_view;

	std::vector<VkBuffer> uniform_buffers;

	std::vector<VkDescriptorSet> descriptor_sets;

	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;

	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	VkImageView texture_image_view;
	VkSampler texture_sampler;

	uint32_t mip_levels;
	VkImage texture_image;
	VkDeviceMemory texture_image_memory;

	template <typename T>
	void CreateVkBuffer(vkpg::VulkanDevice& vulkan_device, const std::vector<T>& input, VkBuffer& buffer,
	                    VkDeviceMemory& buffer_memory, VkBufferUsageFlags usage_flags)
	{
		VkDeviceSize buffer_size = sizeof(input[0]) * input.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		vulkan_device.CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                           staging_buffer, staging_buffer_memory);

		void *data;
		vkMapMemory(vulkan_device.logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
		std::memcpy(data, input.data(), static_cast<size_t>(buffer_size));
		vkUnmapMemory(vulkan_device.logical_device, staging_buffer_memory);

		vulkan_device.CreateBuffer(buffer_size, usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, buffer_memory);

		CopyBuffer(staging_buffer, buffer, buffer_size);

		vkDestroyBuffer(vulkan_device.logical_device, staging_buffer, nullptr);
		vkFreeMemory(vulkan_device.logical_device, staging_buffer_memory, nullptr);
	}
};

} // namespace vkpg
