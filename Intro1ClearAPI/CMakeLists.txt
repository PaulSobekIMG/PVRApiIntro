cmake_minimum_required(VERSION 3.1)
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
set(SOURCE_FILES ClearAPI.cpp)

# Include required framework components 
set(SOURCE_FILES ${SOURCE_FILES} $<TARGET_OBJECTS:PVRShell> $<TARGET_OBJECTS:PVRGlue> $<TARGET_OBJECTS:PVRApi> $<TARGET_OBJECTS:PVRAssets> $<TARGET_OBJECTS:PVRNativeApi> $<TARGET_OBJECTS:PVRCore>)

# Platform specific build and install boilerplate for Windows, Linux, iOS, OSX and Android.

platform_specific_build_and_install()

# Link flags are contained in the Helper.cmake file these can be added to by changing ${FRAMEWORK_LINK_LIBS} variable. All assets are assumed to be in 'Assets' folder contained in this directory