cmake_minimum_required(VERSION 3.20)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

project(arawn VERSION 1.0 LANGUAGES CXX)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(arawn ${SOURCES})
include_directories(${CMAKE_SOURCE_DIR}/inc)

find_package(Vulkan REQUIRED)
if (NOT VULKAN_FOUND) 
	message(FATAL_ERROR "vulkan sdk not found")
endif()

FetchContent_Declare(GLFW GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.4)
FetchContent_Declare(GLM GIT_REPOSITORY https://github.com/g-truc/glm GIT_TAG 1.0.1)
FetchContent_Declare(STB GIT_REPOSITORY https://github.com/nothings/stb GIT_TAG master)
FetchContent_Declare(TINYOBJ GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader GIT_TAG release)
FetchContent_MakeAvailable(GLFW GLM STB TINYOBJ)

target_include_directories(arawn 
	PRIVATE ${Vulkan_INCLUDE_DIRS} 
	PRIVATE ${glfw_SOURCE_DIR}/include
	PRIVATE ${glm_SOURCE_DIR}
	PRIVATE ${stb_SOURCE_DIR}
	PRIVATE ${tinyobjloader_SOURCE_DIR}
)

target_link_libraries(arawn 
	PRIVATE glfw
	PRIVATE tinyobjloader
	PRIVATE Vulkan::Vulkan
)

find_program(GLSLC_EXECUTABLE glslc)

if (GLSLC_EXECUTABLE)
	message(STATUS "ARAWN: using glslc from path")
	set(GLSLC_EXECUTABLE glslc --target-env=vulkan1.2)
elseif(DEFINED ENV{VULKAN_SDK})
	message(STATUS "using vulkan sdk")
	if(WIN32)
		set(GLSLC_EXECUTABLE "$ENV{VULKAN_SDK}\\Bin\\glslc.exe")
	elseif(UNIX)
		set(GLSLC_EXECUTABLE "$ENV{VULKAN_SDK}/Bin/glslc")
	else()
		message(FATAL_ERROR "ARAWN: unsupported platform cannot compile shaders")
	endif()
else()
	message(FATAL_ERROR "ARAWN: couldn't find vulkan sdk")
endif()

if(WIN32)
	string(REPLACE "\\" "/" GLSLC_EXECUTABLE "${GLSLC_EXECUTABLE}")
	string(REPLACE "\\" "/" RESOURCE_DIR "${PROJECT_SOURCE_DIR}/res")
	
	set(IMPORT_DIR "${RESOURCE_DIR}/import/")

	file(GLOB_RECURSE SHADERS "res/*.vert" "res/*.frag" "res/*.comp" "res/*.geom")
elseif(UNIX)
	set(RESOURCE_DIR "${PROJECT_SOURCE_DIR}/res")
	set(IMPORT_DIR "${RESOURCE_DIR}/import")
	
	file(GLOB_RECURSE SHADERS "res/*.vert" "res/*.frag" "res/*.comp" "res/*.geom")
else()
	message(FATAL_ERROR "ARAWN: unsupported platform cannot compile shaders")
endif()

foreach(SHADER ${SHADERS})
	file(RELATIVE_PATH SHADER_DIR ${RESOURCE_DIR} ${SHADER})
	set(TARGET "${IMPORT_DIR}/${SHADER_DIR}.spv")
	
	get_filename_component(TARGET_NAME ${TARGET} NAME_WE)
	string(REPLACE "." "_" TARGET_NAME "${TARGET_NAME}")
	string(REPLACE "/" "_" TARGET_NAME "${TARGET_NAME}")

	if(DEFINED ${TARGET_NAME}_INDEX)
		math(EXPR ${TARGET_NAME}_INDEX "${${TARGET_NAME}_INDEX} + 1")
		set(TARGET_NAME "${TARGET_NAME}_shader_${${TARGET_NAME}_INDEX}")
	else() 
		set(${TARGET_NAME}_INDEX 0)
		set(TARGET_NAME "${TARGET_NAME}_shader")
	endif()

	get_filename_component(TARGET_DIR ${TARGET} DIRECTORY)
	add_custom_command(
		OUTPUT ${TARGET}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGET_DIR}
		COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${TARGET}
		DEPENDS ${SHADER}
		COMMENT "shader compiled: ${SHADER_DIR}"
	)

	add_custom_target(${TARGET_NAME} DEPENDS ${TARGET})
	add_dependencies(arawn ${TARGET_NAME})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SHADERS})
add_dependencies(arawn compile_shaders)