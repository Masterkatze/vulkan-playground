cmake_minimum_required(VERSION 3.18)

project(vulkan-playground LANGUAGES CXX)

find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glfw3 REQUIRED FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE GLSL_SOURCE_FILES "shaders/*.frag" "shaders/*.vert")

add_executable(vulkan-playground
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
  "src/main.cpp"
  ${GLSL_SOURCE_FILES})

target_include_directories(vulkan-playground PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan-playground ${Vulkan_LIBRARIES} glfw)
target_compile_definitions(vulkan-playground PRIVATE
  GLFW_INCLUDE_VULKAN
  STB_IMAGE_IMPLEMENTATION
  TINYOBJLOADER_IMPLEMENTATION
  GLM_ENABLE_EXPERIMENTAL
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(GLSL_VALIDATOR "glslangValidator")

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(vulkan-playground shaders)

add_custom_command(TARGET vulkan-playground PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan-playground>/resources/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/"
        "$<TARGET_FILE_DIR:vulkan-playground>/resources/")
