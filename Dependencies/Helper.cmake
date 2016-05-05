function(platform_specific_build_and_install)
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
	add_executable (${DEMO_NAME} MACOSX_BUNDLE ${SOURCE_FILES})
	target_link_libraries (${DEMO_NAME} ${FRAMEWORK_LINK_LIBS})
	
	set_target_properties(${DEMO_NAME} PROPERTIES COMPILE_FLAGS "-std=c++11")
	
	if (IOS)
		set_target_properties(${DEMO_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/../Dependencies/ios/ios.plist)
	else()
		set_target_properties(${DEMO_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/../Dependencies/osx/osxplist.info)
	endif()
	
	
	if (IOS)
		add_custom_command(TARGET ${DEMO_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_BINARY_DIR}/\${CONFIGURATION}-iphoneos/${DEMO_NAME}.app/Assets)
	else()
		add_custom_command(TARGET ${DEMO_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/../Dependencies/osx/Bundle/Resources ${CMAKE_BINARY_DIR}/\${CONFIGURATION}/${DEMO_NAME}.app/Contents/Resources)
		add_custom_command(TARGET ${DEMO_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/../Dependencies/osx/Bundle/Frameworks ${CMAKE_BINARY_DIR}/\${CONFIGURATION}/${DEMO_NAME}.app/Contents/Frameworks)
		add_custom_command(TARGET ${DEMO_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_BINARY_DIR}/\${CONFIGURATION}/${DEMO_NAME}.app/Contents/Resources/Assets)
	endif()
	
else()
	add_executable (${DEMO_NAME} WIN32 ${SOURCE_FILES})
	target_compile_features(${DEMO_NAME} PRIVATE cxx_range_for)
	target_link_libraries (${DEMO_NAME} ${FRAMEWORK_LINK_LIBS})
	add_custom_command(TARGET ${DEMO_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets Assets)
endif()


install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Assets 
        DESTINATION Assets
)

target_include_directories(${DEMO_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../Dependencies/Native_SDK/Framework)
endfunction(platform_specific_build_and_install)