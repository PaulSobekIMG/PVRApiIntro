cmake_minimum_required(VERSION 3.0)

# This cmake script converts between glsl <-> Vulkan it assume a target of glsl 300 - change as appropriate 
file(STRINGS ${shader_file} contents)

# Get version string of shader assumes Vulkan or glsl for now..
list (GET contents 0 firstLine)

if (${VULKAN})
	if (${firstLine} MATCHES "#version 450.*")
		# No conversion required!
		execute_process(COMMAND ${GLSLANG_EXECUTABLE} -V -o ${target} ${shader_file})				
	else()
		# Convert to temp file and then run spirv compiler
		
		message("Warning - this shader is being converted to Vulkan by chaning the version string to #version 450")
		
		# Get shader extension - glslang uses this to determine shader type
		string(REGEX MATCH "[^.]*$" shader_extension ${shader_file})

		set(tempFile "${target}.temp.${shader_extension}")
		
		FILE(WRITE ${tempFile} "#version 450\n")
		
		FOREACH(line ${contents})
			if (NOT ${line} MATCHES "#version")
				FILE(APPEND ${tempFile} "${line}\n")
			endif()
		ENDFOREACH(line)
		
		execute_process(COMMAND ${GLSLANG_EXECUTABLE} -V -o ${target} ${tempFile})				
		FILE(REMOVE ${tempFile})
	endif()
else()
	if (${firstLine} MATCHES "#version 450.*")
	# Convert on-the-fly to destination
		
		message("Warning - this shader is being converted to OGLES by chaning the version string to #version 300 es")
		
		FILE(WRITE ${target} "#version 300 es\n")
		
		FOREACH(line ${contents})
			if (NOT ${line} MATCHES "#version")
				FILE(APPEND ${target} "${line}\n")
			endif()
		ENDFOREACH(line)
	else()
		# No conversion required
		execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${shader_file} ${target})	
	endif()

endif()
