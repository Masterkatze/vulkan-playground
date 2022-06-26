#include <vulkan/vulkan.h>

#include "utils.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include "debug.hpp"
#include "camera.hpp"
#include "events.hpp"

#include "tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

constexpr auto MODEL_PATH = "resources/models/viking_room.obj";

class Application
{
public:
	Application() :
	    vulkan_device(instance, swap_chain, surface),
	    swap_chain(vulkan_device, window, surface),
	    window(swap_chain, surface, instance),
	    camera(), events(camera)
	{};

	void Run()
	{
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

	vkpg::Camera camera;
	vkpg::Events events;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	std::vector<VkFence> images_in_flight;
	size_t current_frame = 0;

	bool framebuffer_resized = false;

	void InitVulkan()
	{
		window.Init();
		CreateInstance();
		vkpg::Debug::SetupDebugging(instance);
		window.CreateSurface();
		window.SetFramebufferResizeCallback([this](void */*window*/, int width, int height)
		{
			framebuffer_resized = true;

			if(width > 0 && height > 0)
			{
				camera.UpdateAspectRatio(static_cast<float>(width) / static_cast<float>(height));
			}
		});
		window.SetKeyCallback([this](void *window, int key, int scancode, int action, int mods)
		{
			if(ImGui::GetIO().WantCaptureKeyboard)
			{
				return;
			}

			events.KeyCallback(window, key, scancode, action, mods);
		});
		window.SetMouseButtonCallback([this](void *window, int button, int action, int mods)
		{
			if(ImGui::GetIO().WantCaptureMouse)
			{
				return;
			}

			events.MouseButtonCallback(window, button, action, mods);
		});
		window.SetCursorPositionCallback([this](void *window, int x, int y)
		{
			events.CursorPositionCallback(window, x, y);
		});

		vulkan_device.PickPhysicalDevice();
		vulkan_device.CreateLogicalDevice();
		swap_chain.Create();
		swap_chain.CreateImageViews();
		swap_chain.CreateRenderPass();
		swap_chain.CreateUiRenderPass();
		swap_chain.CreateDescriptorSetLayout();
		swap_chain.CreateGraphicsPipeline();
		swap_chain.CreateCommandPool();
		swap_chain.CreateUiCommandPool();
		swap_chain.CreateColorResources();
		swap_chain.CreateDepthResources();
		swap_chain.CreateFramebuffers();
		swap_chain.CreateUiFramebuffers();
		swap_chain.CreateTextureImage();
		swap_chain.CreateTextureImageView();
		swap_chain.CreateTextureSampler();
		LoadModel();
		swap_chain.CreateVertexBuffer();
		swap_chain.CreateIndexBuffer();
		swap_chain.CreateUniformBuffers();
		swap_chain.CreateDescriptorPool();
		swap_chain.CreateUiDescriptorPool();
		swap_chain.CreateDescriptorSets();
		swap_chain.CreateCommandBuffers();
		swap_chain.CreateUiCommandBuffers();
		CreateSyncObjects();

		//InitImGui();
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGui::StyleColorsDark();

			auto& io = ImGui::GetIO();
			int width = 0, height = 0;
			window.GetFramebufferSize(width, height);
			io.DisplaySize = ImVec2(width, height);
			io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

			ImGui_ImplGlfw_InitForVulkan(window.GetNativeHandler(), true);
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = instance;
			init_info.PhysicalDevice = vulkan_device.physical_device;
			init_info.Device = vulkan_device.logical_device;
			init_info.QueueFamily = vulkan_device.queue_family_indices.present_family.value();
			init_info.Queue = swap_chain.graphics_queue;
			init_info.PipelineCache = swap_chain.pipeline_cache;
			init_info.DescriptorPool = swap_chain.ui_descriptor_pool;
			init_info.Subpass = 0;
			init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
			init_info.ImageCount = swap_chain.image_count;
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			init_info.Allocator = nullptr;
			init_info.CheckVkResultFn = [](VkResult result)
			{
				CheckVkResult(result);
			};
			ImGui_ImplVulkan_Init(&init_info, swap_chain.ui_render_pass);

			VkCommandBuffer command_buffer = swap_chain.BeginSingleTimeCommands(swap_chain.ui_command_pool);
			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
			swap_chain.EndSingleTimeCommands(swap_chain.ui_command_pool, command_buffer);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void MainLoop()
	{
		camera.type = vkpg::Camera::CameraType::firstperson;
		camera.SetPosition(glm::vec3(1.0f, 0.75f, 0.0f));

		camera.SetRotation(glm::vec3(0.0f, 90.0f, 0.0f));
		camera.SetPerspective(90.0f, static_cast<float>(swap_chain.extent.width) / static_cast<float>(swap_chain.extent.height), 0.1f, 256.0f);
		camera.SetMovementSpeed(0.02f);

		auto time_last = std::chrono::high_resolution_clock::now();
		while(!window.ShouldClose())
		{
			window.PollEvents();

			ImGui_ImplGlfw_NewFrame();
			ImGui_ImplVulkan_NewFrame();
			ImGui::NewFrame();

			//ImGui::ShowDemoWindow();

			ImGui::SetNextWindowSize(ImVec2(400, 80), ImGuiCond_FirstUseEver);
			ImGui::Begin("Camera");

			float rotation[3] = { camera.rotation.x, camera.rotation.y, camera.rotation.z };
			ImGui::InputFloat3("Rotation", rotation);
			camera.SetRotation({rotation[0], rotation[1], rotation[2]});

			float position[3] = { camera.position.x, camera.position.y, camera.position.z };
			ImGui::InputFloat3("Position", position);
			camera.SetPosition({position[0], position[1], position[2]});

			{
				ImGui::Text("Perspective");

				float matrices_perspective_0[4] =
				{
					camera.matrices.perspective[0][0],
					camera.matrices.perspective[0][1],
					camera.matrices.perspective[0][2],
					camera.matrices.perspective[0][3]
				};
				ImGui::InputFloat4("", matrices_perspective_0);
				camera.matrices.perspective[0][0] = matrices_perspective_0[0];
				camera.matrices.perspective[0][1] = matrices_perspective_0[1];
				camera.matrices.perspective[0][2] = matrices_perspective_0[2];
				camera.matrices.perspective[0][3] = matrices_perspective_0[3];

				float matrices_perspective_1[4] =
				{
					camera.matrices.perspective[1][0],
					camera.matrices.perspective[1][1],
					camera.matrices.perspective[1][2],
					camera.matrices.perspective[1][3]
				};
				ImGui::InputFloat4("", matrices_perspective_1);

				float matrices_perspective_2[4] =
				{
					camera.matrices.perspective[2][0],
					camera.matrices.perspective[2][1],
					camera.matrices.perspective[2][2],
					camera.matrices.perspective[2][3]
				};
				ImGui::InputFloat4("", matrices_perspective_2);

				float matrices_perspective_3[4] =
				{
					camera.matrices.perspective[3][0],
					camera.matrices.perspective[3][1],
					camera.matrices.perspective[3][2],
					camera.matrices.perspective[3][3]
				};
				ImGui::InputFloat4("", matrices_perspective_3);
			}

			{
				ImGui::Text("View");

				float matrices_view_0[4] =
				{
					camera.matrices.view[0][0],
					camera.matrices.view[0][1],
					camera.matrices.view[0][2],
					camera.matrices.view[0][3]
				};
				ImGui::InputFloat4("", matrices_view_0);

				float matrices_view_1[4] =
				{
					camera.matrices.view[1][0],
					camera.matrices.view[1][1],
					camera.matrices.view[1][2],
					camera.matrices.view[1][3]
				};
				ImGui::InputFloat4("", matrices_view_1);

				float matrices_view_2[4] =
				{
					camera.matrices.view[2][0],
					camera.matrices.view[2][1],
					camera.matrices.view[2][2],
					camera.matrices.view[2][3]
				};
				ImGui::InputFloat4("", matrices_view_2);

				float matrices_view_3[4] =
				{
					camera.matrices.view[3][0],
					camera.matrices.view[3][1],
					camera.matrices.view[3][2],
					camera.matrices.view[3][3]
				};
				ImGui::InputFloat4("", matrices_view_3);
			}

			ImGui::End();


			ImGui::Render();

			auto time_now = std::chrono::high_resolution_clock::now();
			auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_last - time_now);
			camera.Update(delta_time);

			DrawFrame();
		}

		vkDeviceWaitIdle(vulkan_device.logical_device);
	}

	void Cleanup()
	{
		// TODO: move to imgui.cleanup
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		swap_chain.Cleanup();

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(vulkan_device.logical_device, render_finished_semaphores[i], nullptr);
			vkDestroySemaphore(vulkan_device.logical_device, image_available_semaphores[i], nullptr);
			vkDestroyFence(vulkan_device.logical_device, in_flight_fences[i], nullptr);
		}

		vulkan_device.Cleanup();

		vkpg::Debug::TearDownDebugging(instance);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		window.Cleanup();
	}

	void CreateInstance()
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Vulkan Playground";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		auto required_extensions = vkpg::VulkanWindow::GetRequiredExtensions();
		create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
		create_info.ppEnabledExtensionNames = required_extensions.data();

		if(false)
		{
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
		}

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
		CheckVkResult(result, "Failed to create instance");
	}

	void LoadModel()
	{
//        auto AddVertex = [this](vkpg::Vertex vertex)
//		{
//			static uint32_t index = 0;
//			swap_chain.vertices.push_back(vertex);
//			swap_chain.indices.push_back(index++);
//		};

//		AddVertex({{0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}});
//		AddVertex({{0.1f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
//		AddVertex({{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}});

//		return;

//        swap_chain.vertices =
//        {
//            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
//            {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
//            {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
//        };

//        swap_chain.indices =
//        {
//            0, 1, 2, 2, 3, 0
//        };


//        return;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH))
		{
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<vkpg::Vertex, uint32_t> unique_vertices{};

		for(const auto& shape : shapes)
		{
			for(const auto& index : shape.mesh.indices)
			{
				vkpg::Vertex vertex{};

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
		images_in_flight.resize(swap_chain.images.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto result = vkCreateSemaphore(vulkan_device.logical_device, &semaphore_info, nullptr, &image_available_semaphores[i]);
			CheckVkResult(result, "Failed to create semaphore");

			result = vkCreateSemaphore(vulkan_device.logical_device, &semaphore_info, nullptr, &render_finished_semaphores[i]);
			CheckVkResult(result, "Failed to create semaphore");

			result = vkCreateFence(vulkan_device.logical_device, &fence_info, nullptr, &in_flight_fences[i]);
			CheckVkResult(result, "Failed to create fence");
		}
	}

	void UpdateUniformBuffer(uint32_t current_image)
	{
        //static auto start_time = std::chrono::high_resolution_clock::now();
        //auto current_time = std::chrono::high_resolution_clock::now();
        //float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

        vkpg::UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
        //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		ubo.view = camera.matrices.view;
        //ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		ubo.projection = camera.matrices.perspective;

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

		//recordUICommands(image_index);
		{
			VkCommandBufferBeginInfo cmdBufferBegin = {};
		    cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		    cmdBufferBegin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		    if (vkBeginCommandBuffer(swap_chain.ui_command_buffers[image_index], &cmdBufferBegin) != VK_SUCCESS)
			{
		        throw std::runtime_error("Unable to start recording UI command buffer!");
		    }

			VkClearValue clearColor{0.0f, 0.0f, 0.0f, 1.0f};
		    VkRenderPassBeginInfo renderPassBeginInfo = {};
		    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		    renderPassBeginInfo.renderPass = swap_chain.ui_render_pass;
		    renderPassBeginInfo.framebuffer = swap_chain.ui_framebuffers[image_index];
		    renderPassBeginInfo.renderArea.extent.width = swap_chain.extent.width;
		    renderPassBeginInfo.renderArea.extent.height = swap_chain.extent.height;
		    renderPassBeginInfo.clearValueCount = 1;
		    renderPassBeginInfo.pClearValues = &clearColor;

		    vkCmdBeginRenderPass(swap_chain.ui_command_buffers[image_index], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		    // Grab and record the draw data for Dear Imgui
		    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), swap_chain.ui_command_buffers[image_index]);

		    // End and submit render pass
		    vkCmdEndRenderPass(swap_chain.ui_command_buffers[image_index]);


		    if(vkEndCommandBuffer(swap_chain.ui_command_buffers[image_index]) != VK_SUCCESS)
			{
		        throw std::runtime_error("Failed to record command buffers!");
		    }
		}

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

		std::array<VkCommandBuffer, 2> command_buffers
		{{
			swap_chain.command_buffers[image_index],
			swap_chain.ui_command_buffers[image_index]
		}};

		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = command_buffers.size();
		submit_info.pCommandBuffers = command_buffers.data();

		VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;

		vkResetFences(vulkan_device.logical_device, 1, &in_flight_fences[current_frame]);

		result = vkQueueSubmit(swap_chain.graphics_queue, 1, &submit_info, in_flight_fences[current_frame]);
		CheckVkResult(result, "Failed to submit draw command buffer");

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;

		std::array<VkSwapchainKHR, 1> swap_chains{{swap_chain.swap_chain}};
		present_info.swapchainCount = swap_chains.size();
		present_info.pSwapchains = swap_chains.data();

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
