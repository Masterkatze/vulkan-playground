#include "swapchain.hpp"
#include "utils.hpp"

#include "stb_image.h"

#include <glm/glm.hpp>

#include <array>

vkpg::VulkanSwapChain::VulkanSwapChain(VulkanDevice& vulkan_device, VulkanWindow& window, VkSurfaceKHR& surface) : vulkan_device(vulkan_device), window(window), surface(surface)
{

}

void vkpg::VulkanSwapChain::Create()
{
	SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(vulkan_device.physical_device);

	VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
	VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
	{
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;

	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto queue_family_indices = vulkan_device.FindQueueFamilies(vulkan_device.physical_device);
	uint32_t queue_family_indices_array[] = {queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value()};

	if(queue_family_indices.graphics_family != queue_family_indices.present_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices_array;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	auto result = vkCreateSwapchainKHR(vulkan_device.logical_device, &create_info, nullptr, &swap_chain);
	CHECK_VKRESULT(result, "Failed to create swap chain");

	vkGetSwapchainImagesKHR(vulkan_device.logical_device, swap_chain, &image_count, nullptr);
	swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(vulkan_device.logical_device, swap_chain, &image_count, swap_chain_images.data());

	swap_chain_image_format = surface_format.format;
	swap_chain_extent = extent;

	msaa_samples = vulkan_device.GetMaxUsableSampleCount();
}

void vkpg::VulkanSwapChain::Cleanup()
{
	vkDestroyImageView(vulkan_device.logical_device, depth_image_view, nullptr);
	vkDestroyImage(vulkan_device.logical_device, depth_image, nullptr);
	vkFreeMemory(vulkan_device.logical_device, depth_image_memory, nullptr);

	vkDestroyImageView(vulkan_device.logical_device, color_image_view, nullptr);
	vkDestroyImage(vulkan_device.logical_device, color_image, nullptr);
	vkFreeMemory(vulkan_device.logical_device, color_image_memory, nullptr);

	for(const auto& framebuffer : swap_chain_framebuffers)
	{
		vkDestroyFramebuffer(vulkan_device.logical_device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(vulkan_device.logical_device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());

	vkDestroyPipeline(vulkan_device.logical_device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(vulkan_device.logical_device, pipeline_layout, nullptr);
	vkDestroyRenderPass(vulkan_device.logical_device, render_pass, nullptr);

	for(const auto& image_view : swap_chain_image_views)
	{
		vkDestroyImageView(vulkan_device.logical_device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(vulkan_device.logical_device, swap_chain, nullptr);

	for(size_t i = 0; i < swap_chain_images.size(); i++)
	{
		vkDestroyBuffer(vulkan_device.logical_device, uniform_buffers[i], nullptr);
		vkFreeMemory(vulkan_device.logical_device, uniform_buffers_memory[i], nullptr);
	}

	vkDestroyDescriptorPool(vulkan_device.logical_device, descriptor_pool, nullptr);

	vkDestroySampler(vulkan_device.logical_device, texture_sampler, nullptr);
	vkDestroyImageView(vulkan_device.logical_device, texture_image_view, nullptr);

	vkDestroyImage(vulkan_device.logical_device, texture_image, nullptr);
	vkFreeMemory(vulkan_device.logical_device, texture_image_memory, nullptr);

	vkDestroyDescriptorSetLayout(vulkan_device.logical_device, descriptor_set_layout, nullptr);

	vkDestroyBuffer(vulkan_device.logical_device, index_buffer, nullptr);
	vkFreeMemory(vulkan_device.logical_device, index_buffer_memory, nullptr);

	vkDestroyBuffer(vulkan_device.logical_device, vertex_buffer, nullptr);
	vkFreeMemory(vulkan_device.logical_device, vertex_buffer_memory, nullptr);
}

void vkpg::VulkanSwapChain::Recreate()
{
	int width = 0, height = 0;
	window.GetFramebufferSize(width, height);
	while(width == 0 || height == 0)
	{
		window.GetFramebufferSize(width, height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vulkan_device.logical_device);

	Cleanup();
	Create();

	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
}

void vkpg::VulkanSwapChain::CreateImageViews()
{
	swap_chain_image_views.resize(swap_chain_images.size());

	for(size_t i = 0; i < swap_chain_images.size(); i++)
	{
		swap_chain_image_views[i] = CreateImageView(swap_chain_images[i], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void vkpg::VulkanSwapChain::CreateRenderPass()
{
	VkAttachmentDescription color_attachment{};
	color_attachment.format = swap_chain_image_format;
	color_attachment.samples = msaa_samples;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = FindDepthFormat();
	depth_attachment.samples = msaa_samples;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref{};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription color_attachment_resolve{};
	color_attachment_resolve.format = swap_chain_image_format;
	color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_resolve_ref{};
	color_attachment_resolve_ref.attachment = 2;
	color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	subpass.pResolveAttachments = &color_attachment_resolve_ref;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = {color_attachment, depth_attachment, color_attachment_resolve};
	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	auto result = vkCreateRenderPass(vulkan_device.logical_device, &render_pass_info, nullptr, &render_pass);
	CHECK_VKRESULT(result, "Failed to create render pass");
}

void vkpg::VulkanSwapChain::CreateGraphicsPipeline()
{
	auto vert_shader_code = ReadFile("shaders/shader.vert.spv");
	auto frag_shader_code = ReadFile("shaders/shader.frag.spv");

	VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
	VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

	auto binding_description = Vertex::GetBindingDescription();
	auto attribute_descriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swap_chain_extent.width);
	viewport.height = static_cast<float>(swap_chain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swap_chain_extent;

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.minSampleShading = 0.2f; // min fraction for sample shading; closer to 1 is smoother
	multisampling.rasterizationSamples = msaa_samples;

	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
											VK_COLOR_COMPONENT_G_BIT |
											VK_COLOR_COMPONENT_B_BIT |
											VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;

	auto result = vkCreatePipelineLayout(vulkan_device.logical_device, &pipeline_layout_info, nullptr, &pipeline_layout);
	CHECK_VKRESULT(result, "Failed to create pipeline layout");

	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.layout = pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(vulkan_device.logical_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline);
	CHECK_VKRESULT(result, "Failed to create graphics pipeline");

	vkDestroyShaderModule(vulkan_device.logical_device, frag_shader_module, nullptr);
	vkDestroyShaderModule(vulkan_device.logical_device, vert_shader_module, nullptr);
}

void vkpg::VulkanSwapChain::CreateColorResources()
{
	VkFormat color_format = swap_chain_image_format;

	CreateImage(swap_chain_extent.width, swap_chain_extent.height, 1, msaa_samples, color_format, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, color_image, color_image_memory);

	color_image_view = CreateImageView(color_image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void vkpg::VulkanSwapChain::CreateDepthResources()
{
	VkFormat depth_format = FindDepthFormat();

	CreateImage(swap_chain_extent.width, swap_chain_extent.height, 1, msaa_samples, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
	depth_image_view = CreateImageView(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void vkpg::VulkanSwapChain::CreateFramebuffers()
{
	swap_chain_framebuffers.resize(swap_chain_image_views.size());

	for(size_t i = 0; i < swap_chain_image_views.size(); i++)
	{
		std::array<VkImageView, 3> attachments =
		{
			color_image_view,
			depth_image_view,
			swap_chain_image_views[i]
		};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = swap_chain_extent.width;
		framebuffer_info.height = swap_chain_extent.height;
		framebuffer_info.layers = 1;

		auto result = vkCreateFramebuffer(vulkan_device.logical_device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]);
		CHECK_VKRESULT(result, "Failed to create framebuffer");
	}
}

void vkpg::VulkanSwapChain::CreateUniformBuffers()
{
	VkDeviceSize buffer_size = sizeof(UniformBufferObject);

	uniform_buffers.resize(swap_chain_images.size());
	uniform_buffers_memory.resize(swap_chain_images.size());

	for(size_t i = 0; i < swap_chain_images.size(); i++)
	{
		vulkan_device.CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 uniform_buffers[i], uniform_buffers_memory[i]);
	}
}

void vkpg::VulkanSwapChain::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(swap_chain_images.size());
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(swap_chain_images.size());

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(swap_chain_images.size());

	auto result = vkCreateDescriptorPool(vulkan_device.logical_device, &pool_info, nullptr, &descriptor_pool);
	CHECK_VKRESULT(result, "Failed to create descriptor pool");
}

void vkpg::VulkanSwapChain::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(swap_chain_images.size(), descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(swap_chain_images.size());
	alloc_info.pSetLayouts = layouts.data();

	descriptor_sets.resize(swap_chain_images.size());
	auto result = vkAllocateDescriptorSets(vulkan_device.logical_device, &alloc_info, descriptor_sets.data());
	CHECK_VKRESULT(result, "Failed to allocate descriptor sets");

	for(size_t i = 0; i < swap_chain_images.size(); i++)
	{
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = texture_image_view;
		image_info.sampler = texture_sampler;

		std::array<VkWriteDescriptorSet, 2> descriptor_writes{};

		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = descriptor_sets[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = descriptor_sets[i];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pImageInfo = &image_info;

		vkUpdateDescriptorSets(vulkan_device.logical_device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
	}
}

void vkpg::VulkanSwapChain::CreateCommandBuffers()
{
	command_buffers.resize(swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

	auto result = vkAllocateCommandBuffers(vulkan_device.logical_device, &alloc_info, command_buffers.data());
	CHECK_VKRESULT(result, "Failed to allocate command buffers");

	for(size_t i = 0; i < command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		result = vkBeginCommandBuffer(command_buffers[i], &begin_info);
		CHECK_VKRESULT(result, "Failed to begin recording command buffer");

		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = render_pass;
		render_pass_info.framebuffer = swap_chain_framebuffers[i];
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = swap_chain_extent;
		render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

		VkBuffer vertex_buffers[] = {vertex_buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);
		vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(command_buffers[i]);

		result = vkEndCommandBuffer(command_buffers[i]);
		CHECK_VKRESULT(result, "Failed to record command buffer");
	}
}

void vkpg::VulkanSwapChain::CreateVertexBuffer()
{
	CreateVkBuffer(vulkan_device, vertices, vertex_buffer, vertex_buffer_memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void vkpg::VulkanSwapChain::CreateIndexBuffer()
{
	CreateVkBuffer(vulkan_device, indices, index_buffer, index_buffer_memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void vkpg::VulkanSwapChain::CreateCommandPool()
{
	auto queue_family_indices = vulkan_device.FindQueueFamilies(vulkan_device.physical_device);

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

	auto result = vkCreateCommandPool(vulkan_device.logical_device, &pool_info, nullptr, &command_pool);
	CHECK_VKRESULT(result, "Failed to create command pool");
}

void vkpg::VulkanSwapChain::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.pImmutableSamplers = nullptr;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding sampler_layout_binding{};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_layout_binding, sampler_layout_binding};
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	auto result = vkCreateDescriptorSetLayout(vulkan_device.logical_device, &layout_info, nullptr, &descriptor_set_layout);
	CHECK_VKRESULT(result, "Failed to create descriptor set layout");
}

const std::string TEXTURE_PATH = "resources/textures/viking_room.png";

void vkpg::VulkanSwapChain::CreateTextureImage()
{
	int tex_width, tex_height, tex_channels;

	stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	if(!pixels)
	{
		throw std::runtime_error("Failed to load texture image \"" + TEXTURE_PATH + "\"");
	}

	VkDeviceSize image_size = tex_width * tex_height * 4;
	mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	vulkan_device.CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	vkMapMemory(vulkan_device.logical_device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(image_size));
	vkUnmapMemory(vulkan_device.logical_device, staging_buffer_memory);

	stbi_image_free(pixels);

	CreateImage(tex_width, tex_height, mip_levels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

	TransitionImageLayout(texture_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
	CopyBufferToImage(staging_buffer, texture_image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	vkDestroyBuffer(vulkan_device.logical_device, staging_buffer, nullptr);
	vkFreeMemory(vulkan_device.logical_device, staging_buffer_memory, nullptr);

	GenerateMipmaps(texture_image, VK_FORMAT_R8G8B8A8_SRGB, tex_width, tex_height, mip_levels);
}

void vkpg::VulkanSwapChain::CreateTextureImageView()
{
	texture_image_view = CreateImageView(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels);
}

void vkpg::VulkanSwapChain::CreateTextureSampler()
{
	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16.0f;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = static_cast<float>(mip_levels);
	sampler_info.mipLodBias = 0.0f;
	auto result = vkCreateSampler(vulkan_device.logical_device, &sampler_info, nullptr, &texture_sampler);
	CHECK_VKRESULT(result, "Failed to create texture sampler");
}

vkpg::VulkanSwapChain::SwapChainSupportDetails vkpg::VulkanSwapChain::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if(format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if(present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR vkpg::VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	auto it = std::find_if(available_formats.begin(), available_formats.end(), [](const auto& available_format)
	{
		return available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	});

	return it != available_formats.end() ? *it : available_formats[0];
}

VkPresentModeKHR vkpg::VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	auto it = std::find_if(available_present_modes.begin(), available_present_modes.end(), [](const auto& available_present_mode)
	{
		return available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR;
	});

	return it != available_present_modes.end() ? *it : VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vkpg::VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if(capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		window.GetFramebufferSize(width, height);

		VkExtent2D actual_extent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actual_extent.width = std::max(capabilities.minImageExtent.width,
									   std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height,
									   std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

VkShaderModule vkpg::VulkanSwapChain::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	auto result = vkCreateShaderModule(vulkan_device.logical_device, &create_info, nullptr, &shader_module);
	CHECK_VKRESULT(result, "Failed to create shader module");

	return shader_module;
}

VkFormat vkpg::VulkanSwapChain::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for(VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vulkan_device.physical_device, format, &props);

		if((tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures  & features) == features) ||
		   (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat vkpg::VulkanSwapChain::FindDepthFormat()
{
	return FindSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkImageView vkpg::VulkanSwapChain::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, int32_t mip_levels)
{
	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = mip_levels;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VkImageView image_view;
	auto result = vkCreateImageView(vulkan_device.logical_device, &view_info, nullptr, &image_view);
	CHECK_VKRESULT(result, "Failed to create image views");

	return image_view;
}

void vkpg::VulkanSwapChain::CreateImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
				 VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
{
	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mip_levels;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = num_samples;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	auto result = vkCreateImage(vulkan_device.logical_device, &image_info, nullptr, &image);
	CHECK_VKRESULT(result, "Failed to create image");

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(vulkan_device.logical_device, image, &mem_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = vulkan_device.FindMemoryType(mem_requirements.memoryTypeBits, properties);

	result = vkAllocateMemory(vulkan_device.logical_device, &alloc_info, nullptr, &image_memory);
	CHECK_VKRESULT(result, "Failed to allocate image memory");

	vkBindImageMemory(vulkan_device.logical_device, image, image_memory, 0);
}

VkCommandBuffer vkpg::VulkanSwapChain::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(vulkan_device.logical_device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void vkpg::VulkanSwapChain::EndSingleTimeCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(vulkan_device.logical_device, command_pool, 1, &command_buffer);
}

void vkpg::VulkanSwapChain::CopyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size)
{
	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(command_buffer, source, destination, 1, &copyRegion);

	EndSingleTimeCommands(command_buffer);
}

void vkpg::VulkanSwapChain::TransitionImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels)
{
	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(command_buffer);
}

void vkpg::VulkanSwapChain::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(command_buffer);
}

void vkpg::VulkanSwapChain::GenerateMipmaps(VkImage image, VkFormat image_format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels)
{
	// Check if image format supports linear blitting
	VkFormatProperties format_properties;
	vkGetPhysicalDeviceFormatProperties(vulkan_device.physical_device, image_format, &format_properties);

	if(!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mip_width = tex_width;
	int32_t mip_height = tex_height;

	for(uint32_t i = 1; i < mip_levels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mip_width, mip_height, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(command_buffer,
					   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1, &blit,
					   VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		if(mip_width > 1) mip_width /= 2;
		if(mip_height > 1) mip_height /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(command_buffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);

	EndSingleTimeCommands(command_buffer);
}
