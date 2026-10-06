cmake_minimum_required(VERSION 4.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "Do not build inside root source directory.")
endif()

set(project_name "some_gui_project")

project(${project_name} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CheckIncludeFiles)
include(GNUInstallDirs)

#-- -- -- -
#Wayland
#-- -- -- -
find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client)

pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

#-- -- -- -- -- -- -- -
#Wayland Scanner
#-- -- -- -- -- -- -- -

find_program(
    WAYLAND_SCANNER
    NAMES wayland-scanner
    REQUIRED
)

#---Gen Wayland---

set(WLR_XML
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml
)

set(XDG_XML
    ${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml
)

set(WLR_HEADER
    ${GENERATED_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h
)

set(WLR_SOURCE
    ${GENERATED_DIR}/wlr-layer-shell-unstable-v1-protocol.c
)

set(XDG_HEADER
    ${GENERATED_DIR}/xdg-shell-client-protocol.h
)

set(XDG_SOURCE
    ${GENERATED_DIR}/xdg-shell-protocol.c
)

add_custom_command(
    OUTPUT
        ${WLR_HEADER}
        ${WLR_SOURCE}

    COMMAND
        ${WAYLAND_SCANNER}
        client-header
        ${WLR_XML}
        ${WLR_HEADER}

    COMMAND
        ${WAYLAND_SCANNER}
        private-code
        ${WLR_XML}
        ${WLR_SOURCE}

    DEPENDS
        ${WLR_XML}
)

add_custom_command(
    OUTPUT
        ${XDG_HEADER}
        ${XDG_SOURCE}

    COMMAND
        ${WAYLAND_SCANNER}
        client-header
        ${XDG_XML}
        ${XDG_HEADER}

    COMMAND
        ${WAYLAND_SCANNER}
        private-code
        ${XDG_XML}
        ${XDG_SOURCE}

    DEPENDS
        ${XDG_XML}
)

#-- -- -- -- -- -- -- --
#Protocol Library
#-- -- -- -- -- -- -- --

add_library(wayland-protocols
    STATIC
	${WLR_SOURCE}
    ${XDG_SOURCE}
)
target_include_directories(wayland-protocols
    PUBLIC
    ${GENERATED_DIR}
)
target_link_libraries(wayland-protocols
    PUBLIC
    PkgConfig::WAYLAND
)

#-- --
#SDL3
#-- --

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

#-- -- -- -- --
#Dear ImGui
#-- -- -- -- --

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

#SDL3 + OpenGL3 backend
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

#-- -- -- -- -- -- -- --
#Main Application
#-- -- -- -- -- -- -- --

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cc)

add_executable(${project_name} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE 
	${WAYLAND_INCLUDE_DIRS}
	${GENERATED_DIR}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
	SDL3::SDL3
	OpenGL::GL
    imgui
	wayland-protocols
	${WAYLAND_LIBRARIES}
)

install(
	TARGETS ${project_name}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BIN_DIR}
)
