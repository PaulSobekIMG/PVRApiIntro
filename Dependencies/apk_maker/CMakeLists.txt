cmake_minimum_required(VERSION 2.8)
#CMake Script to build for Android
#Required Variables:
#ANDROID_NDK - Path to the NDK (Will try to find the ANDROID_NDK Environment variable)
#ANDROID_SDK - Path to the SDK (Will try to find the ANDROID_SDK Environment variable)
#DEMO_SRC_DIR - Path to the CMakeLists.txt demo to build

# Config types for Debug and Release as these are the only ones supported
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE String "Semicolon seperated list of support build types. Only Debug and Release are valid" FORCE)
elseif(NOT CMAKE_BUILD_TYPE )
   set( CMAKE_BUILD_TYPE "Debug" CACHE PATH "Build Type to use when building the project. Only Debug and Release are Supported" FORCE)
endif()

   set( CMAKE_BUILD_TYPE "Debug" CACHE PATH "Build Type to use when building the project. Only Debug and Release are Supported" FORCE)

if(${API} MATCHES "OGLES")
	set(GLSLANG_EXECUTABLE "NULL")
endif()


# Find the Source Directory. 
find_path(DEMO_SRC_DIR "CMakeLists.txt"
          PATHS "${CMAKE_BINARY_DIR}/../../"
          DOC "Path to Source files")
		  
if(NOT DEMO_SRC_DIR)
    message(FATAL_ERROR "DEMO_SRC_DIR is not found. Please provide link to the Demo you want to build")
endif()

# Determine demo name from directory
string(REGEX MATCHALL "[^/\\]+$" DEMO_NAME ${DEMO_SRC_DIR})

if (NOT DEMO_NAME)
	message(FATAL_ERROR "Could not determine demo name from directory - do not pass with trailing slash")
endif()

set(API "" CACHE String "API to target Vulkan or OGLES")

if (NOT API)
	message(FATAL_ERROR "API has not been specified")
endif()

# Lets create the project now and customise the name based on Demo Name
project(${DEMO_NAME}-AndroidBuild NONE)

#Find the Android NDK - Fail if it not found
find_path(ANDROID_NDK "ndk-build"
          PATHS ENV ANDROID_NDK)
if(NOT ANDROID_NDK)
    message(FATAL_ERROR "ANDROID_NDK not found")
endif()

#Find Android SDK - Fail if not found
find_path(ANDROID_SDK "tools/android"
          NAMES "tools/android.bat"
          PATHS ENV ANDROID_SDK)
if(NOT ANDROID_SDK)
    message(FATAL_ERROR "ANDROID_SDK not found")
endif()

#Find Android toolchain file
find_file(ANDROID_TOOLCHAIN_FILE "android.toolchain.cmake"
          PATHS ${CMAKE_CURRENT_LIST_DIR}
               "${CMAKE_CURRENT_LIST_DIR}/../android-cmake"
          DOC "Path to the Android Toolchain file")
if(NOT ANDROID_TOOLCHAIN_FILE)
    message(FATAL_ERROR "ANDROID_TOOLCHAIN_FILE not found")
endif()

#Find Optional Android Config Folder
find_path(ANDROID_CONFIG_DIR
          NAMES "${DEMO_NAME}.java"
                AndroidManifest.xml.in
                android.cmake
                icon.png
          HINTS "${DEMO_SRC_DIR}/android"
          DOC "Android Specific Configuration"
          NO_DEFAULT_PATH)

#Set some Android defaults
set(ANDROID_NATIVE_API_LEVEL 19 CACHE String "Android Native Level")
set(ANDROID_API_LEVEL ${ANDROID_NATIVE_API_LEVEL} CACHE String "Android Java API Level")
set(ANDROID_ACTIVITY "com.powervr.PVRActivity")
set(ANDROID_HAS_JAVA_CODE "true")

function(get_android_generator generator)
    #Default to Ninja Generator if ninja avaliable otherwise Unix Makefiles
    find_program(NINJA_EXE "ninja")
    message("Ninja: ${NINJA_EXE}")
    if(NINJA_EXE)
        set(${generator} "Ninja" PARENT_SCOPE)
    else()
        set(${generator} "Unix Makefiles" PARENT_SCOPE)
    endif()
endfunction()

if(NOT ANDROID_GENERATOR)
    get_android_generator(GENERATOR)
    message("Default Android CMake Generator: ${GENERATOR}")
    set(ANDROID_GENERATOR ${GENERATOR}
        CACHE String "Generator to use for the Android builds")
    unset(GENERATOR)
endif()

set(ANDROID_ABIS "armeabi-v7a;x86;mips;arm64-v8a;x86_64;mips64"
    CACHE String "List of Supported ABIs to build")

add_custom_target(apk ALL
                  DEPENDS ${CMAKE_BINARY_DIR}/build.xml
                          ${CMAKE_BINARY_DIR}/AndroidManifest.xml
                          ${CMAKE_BINARY_DIR}/res/values/strings.xml)

add_custom_command(TARGET apk PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   ${CMAKE_CURRENT_LIST_DIR}/PVRActivity.java
                   ${CMAKE_BINARY_DIR}/src/com/powervr/PVRActivity.java
                  )

find_file(ANDROID_JAVA_SRC ${DEMO_NAME}.java
          HINTS ${ANDROID_CONFIG_DIR}
          NO_DEFAULT_PATH)
mark_as_advanced(ANDROID_JAVA_SRC)
if(ANDROID_JAVA_SRC)
    #Lets add a pre build script to move the Java Source to the build dir if it has changed
    add_custom_command(TARGET apk PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${ANDROID_JAVA_SRC}
                       ${CMAKE_BINARY_DIR}/src/com/powervr/${DEMO_NAME}/${DEMO_NAME}.java
                      )
    #As we're using our own Java Activity we need to tell the build system
    set(ANDROID_ACTIVITY ${DEMO_NAME})
endif()

#Create the Android Manifest file - use demo specific if it exists
find_file(ANDROID_MANIFEST AndroidManifest.xml.in
          HINTS ${ANDROID_CONFIG_DIR}
          PATHS ${CMAKE_CURRENT_LIST_DIR}
          DOC "Path to Android Manifest Configuration"
          NO_DEFAULT_PATH)
mark_as_advanced(ANDROID_MANIFEST) #Hide the variable away to avoid clutter

#Copy the icon file - use demo specific one if exists
find_file(ANDROID_LAUNCH_ICON icon.png
          HINTS ${ANDROID_CONFIG_DIR}
          PATHS ${CMAKE_CURRENT_LIST_DIR}
          DOC "Path to Launcher Icon"
          NO_DEFAULT_PATH)
mark_as_advanced(ANDROID_LAUNCH_ICON) #Hide the variable away to avoid clutter

#Lets get the icon copied in before the apk is built
add_custom_command(TARGET apk PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   ${ANDROID_LAUNCH_ICON}
                   ${CMAKE_BINARY_DIR}/res/drawable/icon.png
                  )

set(EXTRA_ANDROID_CMAKE_ARGS "")
#Allows custom modification to the environment
find_file(ANDROID_EXTRA_CONFIG android.cmake
          HINTS ${ANDROID_CONFIG_DIR}
          DOC "Path to Project specific Android configurations"
          NO_DEFAULT_PATH)
mark_as_advanced(ANDROID_EXTRA_CONFIG)
include(${ANDROID_EXTRA_CONFIG} OPTIONAL)

#create the string.xml
configure_file(${CMAKE_CURRENT_LIST_DIR}/strings.xml.in
               ${CMAKE_BINARY_DIR}/res/values/strings.xml)

#generate the AndroidManifest.xml
configure_file(${ANDROID_MANIFEST} ${CMAKE_BINARY_DIR}/AndroidManifest.xml)


#Create the SDK project
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/project.properties
                          ${CMAKE_BINARY_DIR}/local.properties
                          ${CMAKE_BINARY_DIR}/proguard-project.txt
                          ${CMAKE_BINARY_DIR}/build.xml
                   COMMAND "${ANDROID_SDK}/tools/android" "update" "project"
                   "--target" "android-${ANDROID_API_LEVEL}"
                   "--path" ${CMAKE_BINARY_DIR}
                   "--name" ${DEMO_NAME}
                   COMMENT "Updating Android Project"
)

if(("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR
   "${CMAKE_CONFIGURATION_TYPES}" MATCHES "Release"))
    #Release mode needs to know where the keystore is so lets put it in ant.properties
    find_file(ANDROID_KEYSTORE "key.store"
              PATHS "${CMAKE_CURRENT_LIST_DIR}"
              )
    set(ANDROID_KEYSTORE_ALIAS "mykey" CACHE STRING "Alias to Keystore")
    set(ANDROID_KEYSTORE_PASSWORD ""
        CACHE STRING "Password for keystore")
    if(NOT ANDROID_KEYSTORE_PASSWORD)
        if("${CMAKE_CONFIGURATION_TYPES}" MATCHES "Release")
            message(WARNING "ANDROID_KEYSTORE_PASSWORD has not been set, please set to ensure release building works")
        else()
            message(WARNING "ANDROID_KEYSTORE_PASSWORD has not been set, please set to ensure release building works")
        endif()
    endif()
    find_file(ANDROID_PROPERTIES "ant.properties.in"
        HINTS ${ANDROID_CONFIG_DIR}
        PATHS ${CMAKE_CURRENT_LIST_DIR}
    )
    configure_file("${ANDROID_PROPERTIES}"
                   "${CMAKE_BINARY_DIR}/ant.properties")
    file(STRINGS "${CMAKE_BINARY_DIR}/ant.properties" ant_properties_password
         REGEX "^key\\.(store|alias)\\.password")
    if(ANDROID_KEYSTORE_PASSWORD AND NOT ant_properties_password )
        file(APPEND "${CMAKE_BINARY_DIR}/ant.properties"
                    "key.store.password=${ANDROID_KEYSTORE_PASSWORD}\n"
                    "key.alias.password=${ANDROID_KEYSTORE_PASSWORD}\n")
    endif()
endif()

#Windows usually doesn't have make on the Path, so use the one the NDK provides
if(${ANDROID_GENERATOR} STREQUAL "Unix Makefiles" AND CMAKE_HOST_WIN32)
    find_program(ANDROID_MAKE_PROGRAM "make.exe" NO_DEFAULT_PATH
                 PATHS "${ANDROID_NDK}/prebuilt/windows-x86_64/bin/"
                       "${ANDROID_NDK}/prebuilt/windows-x86/bin/"
                 NO_DEFAULT_PATH
                )
    if(NOT ANDROID_MAKE_PROGRAM)
        message(SEND_ERROR "Cannot find make.exe - is ANDROID_NDK correct?")
    endif()
endif()

#If Specific Make program specified then lets tell the External projects
if(ANDROID_MAKE_PROGRAM)
    set(MAKE_PROGRAM "-DCMAKE_MAKE_PROGRAM=${ANDROID_MAKE_PROGRAM}")
else()
    set(MAKE_PROGRAM "")
endif()


#we should now have set up a working Android Build environment

include(ExternalProject)

#Create a new CMake Build for each ABI we have requested
foreach(ABI ${ANDROID_ABIS})
    option(ANDROID_${ABI}_BUILD "Build for ${ABI}" ON)
    if(ANDROID_${ABI}_BUILD)
        list(APPEND ANDROID_BUILD_ABIS ${ABI})
        #Run Configure to allow EXTRA_ANDROID_CMAKE_ARGS to use ABI specific variables
        string(CONFIGURE "${EXTRA_ANDROID_CMAKE_ARGS}" ANDROID_CMAKE_ARGS)
        if(ANDROID_${ABI}_TOOLCHAIN)
          set(TOOLCHAIN "-DANDROID_TOOLCHAIN_NAME=${ANDROID_${ABI}_TOOLCHAIN}")
        message(${TOOLCHAIN})
        else()
          set(TOOLCHAIN "")
        endif()
        ExternalProject_Add(JNI_${ABI}
            PREFIX "${CMAKE_BINARY_DIR}/jni/${ABI}"
            DOWNLOAD_COMMAND ""
			BUILD_ALWAYS 1
            SOURCE_DIR ${DEMO_SRC_DIR}
            CMAKE_ARGS
                -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_FILE}
				-DAPI=${API}
				-DGLSLANG_EXECUTABLE=${GLSLANG_EXECUTABLE}
                -DANDROID_NDK=${ANDROID_NDK}
                -DANDROID_ABI=${ABI}
                -DANDROID_STL=c++_static
                -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
                ${TOOLCHAIN}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DLIBRARY_OUTPUT_PATH_ROOT=${CMAKE_BINARY_DIR}
                -DLIBRARY_OUTPUT_PATH=${CMAKE_BINARY_DIR}/obj/local/${ABI}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
                ${MAKE_PROGRAM}
                ${ANDROID_CMAKE_ARGS}
            CMAKE_GENERATOR ${ANDROID_GENERATOR}
            INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/${ABI}
        )
        add_dependencies(apk JNI_${ABI})
        set_target_properties(JNI_${ABI} PROPERTIES
            FOLDER "NDK"
            PROJECT_LABEL "${ABI}"
        )
    endif()
endforeach()

string(REGEX REPLACE ";" " " ANDROID_BUILD_ABIS "${ANDROID_BUILD_ABIS}")
file(WRITE ${CMAKE_BINARY_DIR}/jni/Android.mk "APP_ABI := ${ANDROID_BUILD_ABIS}")
file(WRITE ${CMAKE_BINARY_DIR}/jni/Application.mk "APP_ABI := ${ANDROID_BUILD_ABIS}")


#find and setup ant - used to build and install the apk
find_program(ANT_EXE ant)
if(ANT_EXE)
    #Generate Ant Command: debug if Debug Build or release if Release build
    set(ant_command "$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>")
    #Extra ANT Arguments
    set(ant_args "")
    option(ANT_QUIET "Silence Ant output" ON)
    if(ANT_QUIET)
        set(ant_args ${ant_args} -quiet)
    endif()
    option(ANT_LINT_CHECK "Activate Java Lint Checker" ON)
    if(ANT_LINT_CHECK)
        set(ant_args ${ant_args} -Djava.compilerargs=-Xlint)
    endif()
    set(ANT_EXTRA_ARGS "" CACHE STRING "Extra args to pass to ant")
    mark_as_advanced(ANT_QUIET ANT_LINT_CHECK ANT_EXTRA_ARGS)

    add_custom_command(TARGET apk POST_BUILD
                       COMMAND ${ANT_EXE} ${ant_args} ${ANT_EXTRA_ARGS}  ${ant_command}
                       COMMENT "Building Android APK")

    #Hack to change command based on Build type
    #Kinda undocumented so may break in future cmake updates
    set(install_code
        "if(BUILD_TYPE STREQUAL \"Debug\")"
        "  execute_process(COMMAND ${ANT_EXE} installd)"
        "elseif(BUILD_TYPE STREQUAL \"Release\")"
        "  execute_process(COMMAND ${ANT_EXE} installr)"
        "endif()"
    )
    string(REPLACE ";" "\n" install_code "${install_code}")
    install(CODE ${install_code})
else()
    message(WARNING "Ant not found - apk creation and installation not enabled")
endif()

#find adb - if found we can run the apk by 'building' run
find_program(ADB_EXE adb PATHS "${ANDROID_SDK}/platform-tools")
if(ADB_EXE AND ANT_EXE)
    add_custom_target(run COMMAND
        ${ADB_EXE} shell am start -n com.powervr.${DEMO_NAME}/com.powervr.PVRActivity )
elseif(NOT ADB_EXE)
    message(WARNING "adb not found - disabling ability to run application")
endif()


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
