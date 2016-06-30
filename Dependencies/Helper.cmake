# Store this files directory to find osx/ios build files when calling below function
set(HELPER_DIR ${CMAKE_CURRENT_LIST_DIR})

function(platform_specific_build_and_install)
	
	
	# Start by converting single source shaders based on build type
	set (ASSET_TARGETS "")
	FOREACH(shader ${SINGLE_SOURCE_SHADERS})
		# Single source shaders have their version number replaced to match Vulkan or GL depending on API target
		set(ASSET_TARGETS ${ASSET_TARGETS} ${CMAKE_BINARY_DIR}/${shader})	
		if(${API} MATCHES "OGLES")
			add_custom_command(OUTPUT ${shader}  
				COMMAND "${CMAKE_COMMAND}" "-Dshader_file=${CMAKE_CURRENT_LIST_DIR}/${shader}" "-Dtarget=${CMAKE_BINARY_DIR}/${shader}" "-P" "${HELPER_DIR}/single_source_shader.cmake" 
				DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${shader})
		else()
			# For Vulkan we additionally create spir-v binaries
			if(NOT GLSLANG_EXECUTABLE)
				message(FATAL_ERROR "No spirv compiler ${GLSLANG_EXECUTABLE} provided through -DGLSLANG_EXECUTABLE")
			endif()
			add_custom_command(OUTPUT ${shader} 
				COMMAND "${CMAKE_COMMAND}" "-DGLSLANG_EXECUTABLE=${GLSLANG_EXECUTABLE}" "-Dshader_file=${CMAKE_CURRENT_LIST_DIR}/${shader}" "-Dtarget=${CMAKE_BINARY_DIR}/${shader}" "-DVULKAN=1" "-P" "${HELPER_DIR}/single_source_shader.cmake" DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${shader})
		endif()
	ENDFOREACH(shader)
	
	
	if(${API} MATCHES "Vulkan")
		# Compile Vulkan shaders
		FOREACH(shader ${VULKAN_SHADERS})
			set(ASSET_TARGETS ${ASSET_TARGETS} ${CMAKE_BINARY_DIR}/${shader})	
		
			if(NOT GLSLANG_EXECUTABLE)
				message(FATAL_ERROR "No spirv compiler ${GLSLANG_EXECUTABLE} provided through -DGLSLANG_EXECUTABLE")
			endif()
			add_custom_command(OUTPUT ${shader} 
				COMMAND ${GLSLANG_EXECUTABLE} -V -o ${CMAKE_BINARY_DIR}/${shader} ${CMAKE_CURRENT_LIST_DIR}/${shader} 
				DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${shader})
		ENDFOREACH()
	else()
		# Copy OpenGLES shaders 
		FOREACH(shader ${OPENGLES_SHADERS})
			set(ASSET_TARGETS ${ASSET_TARGETS} ${CMAKE_BINARY_DIR}/${shader})	
			add_custom_command(OUTPUT ${shader} 
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/${shader} ${CMAKE_BINARY_DIR}/${shader} DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${shader})
		ENDFOREACH()
	endif()
	

	# Copy assets into build directory for the benefit of Visual Studio
	FOREACH(asset ${ASSETS})
		set(ASSET_TARGETS ${ASSET_TARGETS} ${CMAKE_BINARY_DIR}/${asset})
		add_custom_command(OUTPUT ${asset} 
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/${asset} ${CMAKE_BINARY_DIR}/${asset} 
			DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${asset})
	ENDFOREACH()
	
	# Create a build target for shaders/assets that is separate
	add_custom_target(AssetBuild ALL 
						DEPENDS ${ASSET_TARGETS}
						SOURCES ${SINGLE_SOURCE_SHADERS} ${VULKAN_SHADERS} ${OPENGLES_SHADERS} ${ASSETS})

	# Platform specific build
	if (ANDROID) # Android
		add_library(${DEMO_NAME} SHARED ${SOURCE_FILES})
		set_target_properties(${DEMO_NAME} PROPERTIES COMPILE_FLAGS "-std=c++11")
		target_link_libraries (${DEMO_NAME} ${FRAMEWORK_LINK_LIBS})
		install(TARGETS ${DEMO_NAME} EXPORT ${DEMO_NAME} DESTINATION libs/${ANDROID_ABI})

		#Create NDK GDB configuration
		get_directory_property(PROJECT_INCLUDES DIRECTORY ${PROJECT_SOURCE_DIR} INCLUDE_DIRECTORIES)
		string(REGEX REPLACE ";" " " PROJECT_INCLUDES "${PROJECT_INCLUDES}")
		file(WRITE ${CMAKE_BINARY_DIR}/gdb.setup
			"set solib-search-path ${CMAKE_INSTALL_PREFIX}/${ANDROID_ABI}\n"
			"directory ${PROJECT_INCLUDES}\n"
		)
		install(FILES ${CMAKE_BINARY_DIR}/gdb.setup DESTINATION libs/${ANDROID_ABI})
		install(FILES ${ANDROID_NDK}/prebuilt/android-${ANDROID_ARCH_NAME}/gdbserver/gdbserver DESTINATION libs/${ANDROID_ABI})
	elseif (APPLE) # iOS/OSX
	
		if (NOT IOS)
			set(OSX_LAYER ${HELPER_DIR}/osx/Bundle/Frameworks/libGLESv2.dylib 
				${HELPER_DIR}/osx/Bundle/Frameworks/libEGL.dylib 
				${HELPER_DIR}/osx/Bundle/Resources/en.lproj/MainMenu.nib)
		endif()
			
		add_executable (${DEMO_NAME} MACOSX_BUNDLE ${SOURCE_FILES} ${ASSET_TARGETS} ${OSX_LAYER})
		
		# Copy in resources
		set_source_files_properties(${ASSET_TARGETS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		
		if (NOT IOS)
			set_source_files_properties(${HELPER_DIR}/osx/Bundle/Resources/en.lproj/MainMenu.nib PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/en.lproj")
			
			# OSX VFrame library
			if (NOT EXISTS ${HELPER_DIR}/osx/Bundle/Frameworks/libEGL.dylib)
				message(FATAL_ERROR "Missing PVRVFrame libraries for OS X - place these in Dependencies/osx/Bundle/Frameworks/")
			endif()
			set_source_files_properties(${HELPER_DIR}/osx/Bundle/Frameworks/libEGL.dylib  PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")
			
			if (NOT EXISTS ${HELPER_DIR}/osx/Bundle/Frameworks/libGLESv2.dylib)
				message(FATAL_ERROR "Missing PVRVFrame libraries for OS X - place these in Dependencies/osx/Bundle/Frameworks/")
			endif()
			set_source_files_properties(${HELPER_DIR}/osx/Bundle/Frameworks/libGLESv2.dylib  PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")
		endif()
		
		target_link_libraries (${DEMO_NAME} ${FRAMEWORK_LINK_LIBS})
		
		set_target_properties(${DEMO_NAME} PROPERTIES COMPILE_FLAGS "-std=c++11")
		
		if (IOS)
			set_target_properties(${DEMO_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${HELPER_DIR}/ios/ios.plist)
		else()
			set_target_properties(${DEMO_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${HELPER_DIR}/osx/osxplist.info)
		endif()
	else()
		# Windows/Linux build
		add_executable (${DEMO_NAME} WIN32 ${SOURCE_FILES})
		target_compile_features(${DEMO_NAME} PRIVATE cxx_range_for)
		target_link_libraries (${DEMO_NAME} ${FRAMEWORK_LINK_LIBS})
	endif()

	# Add assets as a dependency and install for non-Apple OS
	if (NOT APPLE)
		add_dependencies(${DEMO_NAME} AssetBuild)
		install(FILES ${ASSET_TARGETS} 
			DESTINATION Assets)
	endif()
	
	target_include_directories(${DEMO_NAME} PRIVATE ${HELPER_DIR}/Native_SDK/Framework)
	
endfunction(platform_specific_build_and_install)
