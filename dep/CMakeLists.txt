include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

# TODO: move imgui to dep/imgui
FetchContent_Declare(imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui"
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC)

target_sources(imgui
	PRIVATE
	${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/imgui.cpp

	${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui
	PUBLIC
	${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
