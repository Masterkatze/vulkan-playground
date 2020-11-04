#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "tiny_obj_loader.h"

#include "utils.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include "debug.hpp"

#include <iostream>
#include <vector>
#include <set>
#include <optional>
#include <chrono>
#include <unordered_map>

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "resources/models/viking_room.obj";

class Application
{
public:
	Application() : vulkan_device(instance, swap_chain, surface), swap_chain(vulkan_device, window, surface), window(swap_chain, surface, instance) {};

	void Run()
	{
		window.Init();
		InitVulkan();
		MainLoop();
		Cleanup();
	}

private:
	VkInstance instance;
	VkSurfaceKHR surface;

	vkpg::VulkanDevice vulkan_device;
	vkpg::VulkanSwapChain swap_chain;
	vkpg::VulkanWindow window;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	std::vector<VkFence> images_in_flight;
	size_t current_frame = 0;

	bool framebuffer_resized = false;

	void InitVulkan()
	{
		CreateInstance();
		vkpg::Debug::SetupDebugging(instance);
		window.CreateSurface();
		vulkan_device.PickPhysicalDevice();
		vulkan_device.CreateLogicalDevice();
		swap_chain.Create();
		swap_chain.CreateImageViews();
		swap_chain.CreateRenderPass();
		swap_chain.CreateDescriptorSetLayout();
		swap_chain.CreateGraphicsPipeline();
		swap_chain.CreateCommandPool();
		swap_chain.CreateColorResources();
		swap_chain.CreateDepthResources();
		swap_chain.CreateFramebuffers();
		swap_chain.CreateTextureImage();
		swap_chain.CreateTextureImageView();
		swap_chain.CreateTextureSampler();
		LoadModel();
		swap_chain.CreateVertexBuffer();
		swap_chain.CreateIndexBuffer();
		swap_chain.CreateUniformBuffers();
		swap_chain.CreateDescriptorPool();
		swap_chain.CreateDescriptorSets();
		swap_chain.CreateCommandBuffers();
		CreateSyncObjects();
	}

	void MainLoop()
	{
		while(!window.ShouldClose())
		{
			window.PollEvents();
			DrawFrame();
		}

		vkDeviceWaitIdle(vulkan_device.logical_device);
	}

	void Cleanup()
	{
		swap_chain.Cleanup();

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(vulkan_device.logical_device, render_finished_semaphores[i], nullptr);
			vkDestroySemaphore(vulkan_device.logical_device, image_available_semaphores[i], nullptr);
			vkDestroyFence(vulkan_device.logical_device, in_flight_fences[i], nullptr);
		}

		vkDestroyCommandPool(vulkan_device.logical_device, swap_chain.command_pool, nullptr);

		vkDestroyDevice(vulkan_device.logical_device, nullptr);

		vkDestroyInstance(instance, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);

		window.Cleanup();
	}

	void CreateInstance()
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Vulkan Tutorial";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		auto required_extensions = GetRequiredExtensions();
		create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
		create_info.ppEnabledExtensionNames = required_extensions.data();

		std::cout << "Required extensions:" << std::endl;
		for(const auto& extension : required_extensions)
		{
			std::cout << '\t' << extension << std::endl;
		}
		std::cout << std::endl;

		auto available_extensions = GetAvailableExtensions();

		std::cout << "Available extensions:" << std::endl;
		for(const auto& extension : available_extensions)
		{
			std::cout << '\t' << extension.extensionName << " (version " << extension.specVersion << ")" << std::endl;
		}
		std::cout << std::endl;

		VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
		if(vkpg::Debug::enable_validation_layers)
		{
			const auto& validation_layers = vkpg::Debug::GetValidationLayers();
			create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			create_info.ppEnabledLayerNames = validation_layers.data();

			vkpg::Debug::PopulateDebugMessengerCreateInfo(debug_create_info);
			create_info.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info);
		}
		else
		{
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}

		auto result = vkCreateInstance(&create_info, nullptr, &instance);
		CHECK_VKRESULT(result, "Failed to create instance");
	}

	void LoadModel()
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
		{
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> unique_vertices{};

		for(const auto& shape : shapes)
		{
			for(const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};

				vertex.position =
				{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texture_coordinates =
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = {1.0f, 1.0f, 1.0f};
				if(!unique_vertices.contains(vertex))
				{
					unique_vertices[vertex] = static_cast<uint32_t>(swap_chain.vertices.size());
					swap_chain.vertices.push_back(vertex);
				}

				swap_chain.indices.push_back(unique_vertices[vertex]);
			}
		}
	}

	void CreateSyncObjects()
	{
		image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		images_in_flight.resize(swap_chain.swap_chain_images.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto result = vkCreateSemaphore(vulkan_device.logical_device, &semaphore_info, nullptr, &image_available_semaphores[i]);
			CHECK_VKRESULT(result, "Failed to create semaphore");

			result = vkCreateSemaphore(vulkan_device.logical_device, &semaphore_info, nullptr, &render_finished_semaphores[i]);
			CHECK_VKRESULT(result, "Failed to create semaphore");

			result = vkCreateFence(vulkan_device.logical_device, &fence_info, nullptr, &in_flight_fences[i]);
			CHECK_VKRESULT(result, "Failed to create fence");
		}
	}

	void UpdateUniformBuffer(uint32_t current_image)
	{
		static auto start_time = std::chrono::high_resolution_clock::now();

		auto current_time = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.projection = glm::perspective(glm::radians(45.0f), swap_chain.swap_chain_extent.width / static_cast<float>(swap_chain.swap_chain_extent.height), 0.1f, 10.0f);
		ubo.projection[1][1] *= -1;

		void *data;
		vkMapMemory(vulkan_device.logical_device, swap_chain.uniform_buffers_memory[current_image], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(vulkan_device.logical_device, swap_chain.uniform_buffers_memory[current_image]);
	}

	void DrawFrame()
	{
		vkWaitForFences(vulkan_device.logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

		uint32_t image_index;
		auto result = vkAcquireNextImageKHR(vulkan_device.logical_device, swap_chain.swap_chain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

		if(result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swap_chain.Recreate();
			return;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image (VkResult: " + std::to_string(result) + ")");
		}

		UpdateUniformBuffer(image_index);

		// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		if(images_in_flight[image_index] != VK_NULL_HANDLE)
		{
			vkWaitForFences(vulkan_device.logical_device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
		}
		// Mark the image as now being in use by this frame
		images_in_flight[image_index] = in_flight_fences[current_frame];

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
		VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &swap_chain.command_buffers[image_index];

		VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;

		vkResetFences(vulkan_device.logical_device, 1, &in_flight_fences[current_frame]);

		result = vkQueueSubmit(swap_chain.graphics_queue, 1, &submit_info, in_flight_fences[current_frame]);
		CHECK_VKRESULT(result, "Failed to submit draw command buffer");

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;

		VkSwapchainKHR swapChains[] = {swap_chain.swap_chain};
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapChains;

		present_info.pImageIndices = &image_index;

		result = vkQueuePresentKHR(swap_chain.present_queue, &present_info);

		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
		{
			framebuffer_resized = false;
			swap_chain.Recreate();
		}
		else if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image (VkResult: " + std::to_string(result) + ")");
		}

		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t glfw_extension_count = 0;
		const char **glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

		if(vkpg::Debug::enable_validation_layers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	std::vector<VkExtensionProperties> GetAvailableExtensions()
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

		return extensions;
	}
};

int main()
{
	Application app;

	try
	{
		app.Run();
	}
	catch(const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
