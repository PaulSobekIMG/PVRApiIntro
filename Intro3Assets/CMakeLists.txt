cmake_minimum_required(VERSION 3.0)
include(../Dependencies/Helper.cmake)

# Determine project name from directory - this is required for our android apk builder
string(REGEX MATCHALL "[^/\\]+$" DEMO_NAME ${CMAKE_CURRENT_LIST_DIR})

if (NOT DEMO_NAME)
	message(FATAL_ERROR "Could not determine demo name from directory - do not pass with trailing slash")
endif()

project(${DEMO_NAME})

# Include the framework
find_package(PVRFramework REQUIRED PATHS "${CMAKE_CURRENT_LIST_DIR}/../Dependencies/Native_SDK/Framework")

# Set our demo source files
set(SOURCE_FILES Assets.cpp)

# Include required framework components 
set(SOURCE_FILES ${SOURCE_FILES} $<TARGET_OBJECTS:PVRShell> $<TARGET_OBJECTS:PVRGlue> $<TARGET_OBJECTS:PVRApi> $<TARGET_OBJECTS:PVRAssets> $<TARGET_OBJECTS:PVRNativeApi> $<TARGET_OBJECTS:PVRCore>)

# Set our shaders - single source shaders will be converted from glsl to Vulkan by setting #define VULKAN and replacing #version 300 es string to #version 450
set(SINGLE_SOURCE_SHADERS gnome_shader.frag gnome_shader.vert)

# API specific shaders

# VULKAN_SHADERS are compiled using glslangValidator by the build
#set(VULKAN_SHADERS shader.spirv.frag)

# OPENGLES_SHADERS are copied straight to the target
#set(OPENGLES_SHADERS shader.vsh)

# Assets

# ASSETS are copied straight to the target
set(ASSETS gnome_tex_small.pvr gnome0.pod)

# Platform specific build and install boilerplate for Windows, Linux, iOS, OSX and Android.
platform_specific_build_and_install()

# Link flags are contained in the Helper.cmake file these can be added to by changing ${FRAMEWORK_LINK_LIBS} variable.
