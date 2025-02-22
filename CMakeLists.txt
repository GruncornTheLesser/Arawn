cmake_minimum_required(VERSION 3.20)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

project(ARAWN VERSION 1.0 LANGUAGES CXX)
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(ARAWN ${SOURCES})
include_directories(${CMAKE_SOURCE_DIR}/inc)

find_package(Vulkan REQUIRED)
if (NOT VULKAN_FOUND) 
	message(FATAL_ERROR "vulkan sdk not found")
endif()

FetchContent_Declare(GLFW GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.3.8)
FetchContent_Declare(GLM GIT_REPOSITORY https://github.com/g-truc/glm GIT_TAG release-1-0-2)
FetchContent_Declare(ENTT GIT_REPOSITORY https://github.com/skypjack/entt GIT_TAG v3.13.x)
FetchContent_MakeAvailable(GLFW GLM ENTT)

target_include_directories(ARAWN 
	PRIVATE ${Vulkan_INCLUDE_DIRS} 
	PRIVATE ${glfw_SOURCE_DIR}/include
	PRIVATE ${glm_SOURCE_DIR}
	PRIVATE ${entt_SOURCE_DIR}/single_include
)

target_link_libraries(ARAWN 
	PRIVATE glfw
	PRIVATE Vulkan::Vulkan
)

if(WIN32)

	set(GLSLC_EXECUTABLE "$ENV{VULKAN_SDK}\\Bin\\glslc.exe")
	string(REPLACE "\\" "/" GLSLC_EXECUTABLE "${GLSLC_EXECUTABLE}")
	
	string(REPLACE "\\" "/" RESOURCE_DIR "${PROJECT_SOURCE_DIR}")
	
	set(RESOURCE_DIR "${RESOURCE_DIR}/res")
	set(IMPORT_DIR "${RESOURCE_DIR}/import/")
	file(GLOB_RECURSE SHADERS "res/*.vert" "res/*.frag" "res/*.comp" "res/*.geom")
elseif(UNIX)
	set(GLSLC_EXECUTABLE "$ENV{VULKAN_SDK}/Bin/glslc")
	set(RESOURCE_DIR "${PROJECT_SOURCE_DIR}/res")
	set(IMPORT_DIR "${RESOURCE_DIR}/import/") # slash appended
	file(GLOB_RECURSE SHADERS "res/*.vert" "res/*.frag" "res/*.comp" "res/*.geom")
else()
	message(FATAL_ERROR "unsupported platform cannot compile shaders")
endif()

set(SHADER_INDEX 1)
foreach(SHADER ${SHADERS})
	file(RELATIVE_PATH SHADER_DIR ${RESOURCE_DIR} ${SHADER})
	set(TARGET "${IMPORT_DIR}${SHADER_DIR}.spv")
	get_filename_component(TARGET_DIR ${TARGET} DIRECTORY)
	
	add_custom_command(
		OUTPUT ${TARGET}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGET_DIR}
		COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${TARGET}
		DEPENDS ${SHADER}
		COMMENT "shader compiled: ${SHADER_DIR}"
	)

	add_custom_target(compile_shader_${SHADER_INDEX} DEPENDS ${TARGET})
	add_dependencies(ARAWN compile_shader_${SHADER_INDEX})

	math(EXPR SHADER_INDEX "${SHADER_INDEX} + 1")
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SHADERS})
add_dependencies(ARAWN compile_shaders)