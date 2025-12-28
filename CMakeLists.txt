cmake_minimum_required(VERSION 3.20)

project(arawn VERSION 1.0 LANGUAGES CXX)

# --------------------------
# c++ settings
# --------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------
# Engine sources
# --------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "inc/*.h")

# --------------------------
# dependencies
# --------------------------
include(FetchContent)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "")

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.4)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm GIT_TAG 1.0.1)
FetchContent_Declare(vma GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git GIT_TAG v3.3.0)

FetchContent_MakeAvailable(glfw glm vma)

# --------------------------
# create library
# --------------------------
add_library(arawn STATIC ${SOURCES} ${HEADERS})
add_library(arawn::engine ALIAS arawn)

target_include_directories(arawn 
PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/inc 
PRIVATE 
	${glm_SOURCE_DIR} 
	${vma_SOURCE_DIR}/include 
	${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(arawn 
PRIVATE
	glfw
	Vulkan::Vulkan
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(arawn PUBLIC ARAWN_DEBUG=1)
endif()

# --------------------------
# shader compilation
# --------------------------
find_program(GLSLC_EXECUTABLE glslc)
find_program(GLSLANGVALIDATOR glslangValidator)

if (GLSLC_EXECUTABLE)
	message(STATUS "ARAWN: using glslc from path")
	set(GLSLC_EXECUTABLE glslc -S --target-env=vulkan1.2)
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

set(RESOURCE_DIR "${PROJECT_SOURCE_DIR}/res")
set(IMPORT_DIR "${RESOURCE_DIR}/import")

file(GLOB_RECURSE SHADERS
    "${RESOURCE_DIR}/*.vert"
    "${RESOURCE_DIR}/*.frag"
    "${RESOURCE_DIR}/*.comp"
    "${RESOURCE_DIR}/*.geom"
)

set(SPIRV_OUTPUTS "")
foreach(SHADER ${SHADERS})
	file(RELATIVE_PATH REL_PATH ${RESOURCE_DIR} ${SHADER})
	set(OUT_FILE "${IMPORT_DIR}/${REL_PATH}.spv")
    list(APPEND SPIRV_OUTPUTS ${OUT_FILE})
	
	get_filename_component(OUT_DIR ${OUT_FILE} DIRECTORY)
	
    add_custom_command(
		OUTPUT ${OUT_FILE}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
		COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${OUT_FILE}
		DEPENDS ${SHADER}
		COMMENT "compiling shader: ${REL_PATH}"
        VERBATIM
	)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(arawn shaders)
