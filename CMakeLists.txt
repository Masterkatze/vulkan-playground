cmake_minimum_required(VERSION 3.18)

project(vulkan-playground LANGUAGES CXX)

find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glfw3 REQUIRED FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME}
	PRIVATE
	"src/stb_image.h"
	"src/tiny_obj_loader.h"
	"src/device.hpp"
	"src/device.cpp"
	"src/swapchain.hpp"
	"src/swapchain.cpp"
	"src/debug.hpp"
	"src/debug.cpp"
	"src/window.hpp"
	"src/window.cpp"
	"src/utils.hpp"
	"src/utils.cpp"
	"src/camera.hpp"
	"src/camera.cpp"
	"src/main.cpp"
)

file(GLOB_RECURSE GLSL_SOURCE_FILES "shaders/*.frag" "shaders/*.vert")

target_sources(${CMAKE_PROJECT_NAME}
	PRIVATE
	${GLSL_SOURCE_FILES}
)

source_group("Shaders" FILES ${GLSL_SOURCE_FILES})

target_include_directories(${CMAKE_PROJECT_NAME}
	PRIVATE
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
	PRIVATE
	${Vulkan_LIBRARIES}
	glfw
)

target_compile_definitions(${CMAKE_PROJECT_NAME}
	PRIVATE
	GLFW_INCLUDE_VULKAN
	STB_IMAGE_IMPLEMENTATION
	TINYOBJLOADER_IMPLEMENTATION
	GLM_ENABLE_EXPERIMENTAL
	GLM_FORCE_RADIANS
	GLM_FORCE_DEPTH_ZERO_TO_ONE
)

set(GLSL_VALIDATOR "glslangValidator")

foreach(GLSL ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME)
	set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
	)
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(${CMAKE_PROJECT_NAME} shaders)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan-playground>/resources/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/resources/" "$<TARGET_FILE_DIR:vulkan-playground>/resources/")
