cmake_minimum_required(VERSION 4.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "Do not build inside root source directory.")
endif()

set(project_name "some_gui_project")

project(${project_name} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CheckIncludeFiles)
include(GNUInstallDirs)

# -------
# Wayland
# -------
find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

# ----
# SDL3
# ----

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# ----------
# Dear ImGui
# ----------

FetchContent_Declare(
    imgui
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.2.tar.gz
)

FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    # SDL3 + OpenGL3 backend
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    SDL3::SDL3
)

# ----------------
# Main Application
# ----------------

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cc)

add_executable(${project_name} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE 
	${WAYLAND_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
	SDL3::SDL3
	OpenGL::GL
    imgui
	${WAYLAND_LIBRARIES}
)

install(
	TARGETS ${project_name}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BIN_DIR}
)
