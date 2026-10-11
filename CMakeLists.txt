cmake_minimum_required(VERSION 3.20)

execute_process(
	COMMAND git describe --tags --abbrev=0
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ARAWN_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)

math(EXPR ARAWN_VERSION_VARIANT "1") # branch 1
math(EXPR ARAWN_VERSION_MAJOR   "0")
math(EXPR ARAWN_VERSION_MINOR   "0")
math(EXPR ARAWN_VERSION_PATCH   "0")
if(ARAWN_VERSION MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
	math(EXPR ARAWN_VERSION_MAJOR   "${CMAKE_MATCH_1}")
	math(EXPR ARAWN_VERSION_MINOR   "${CMAKE_MATCH_2}")
	math(EXPR ARAWN_VERSION_PATCH   "${CMAKE_MATCH_3}")
endif()

set(ARAWN_VERSION "${ARAWN_VERSION_VARIANT}.${ARAWN_VERSION_MAJOR}.${ARAWN_VERSION_MINOR}.${ARAWN_VERSION_PATCH}")

project(arawn VERSION ${ARAWN_VERSION} LANGUAGES CXX)

message(STATUS "Arawn Version: ${ARAWN_VERSION}")

# --------------------------
# c++ settings
# --------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------
# dependencies
# --------------------------
include(FetchContent)

find_package(Vulkan REQUIRED)
find_package(assimp QUIET)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_VULKAN ON CACHE BOOL "" FORCE)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "")

# set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
# set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)

# set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "" FORCE)
# set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm GIT_TAG 1.0.1)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.4)
FetchContent_Declare(vma GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git GIT_TAG v3.3.0)
# FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
# FetchContent_Declare(meshopt GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git GIT_TAG v0.22)
# FetchContent_Declare(assimp GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v5.4.3)

FetchContent_MakeAvailable(glfw glm vma) # stb meshopt assimp

find_program(GLSLC_EXECUTABLE glslc)
find_program(GLSLANGVALIDATOR glslangValidator)

if (GLSLC_EXECUTABLE)
	message(STATUS "ARAWN: using glslc from path")
	set(GLSLC_EXECUTABLE glslc --target-env=vulkan1.3)
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

# --------------------------
# Engine sources
# --------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "inc/*.h")
file(GLOB_RECURSE SHADERS "res/*.vert"
	"res/*.frag"
	"res/*.comp"
	"res/*.geom"
)

# --------------------------
# shader compilation
# --------------------------
set(SHADER_TARGETS "")
foreach(SHADER ${SHADERS})
	file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/res ${SHADER})
	
	set(OUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/import/${REL_PATH}.spv)
	list(APPEND SHADER_TARGETS ${OUT_FILE})

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

add_custom_target(shaders DEPENDS ${SHADER_TARGETS})

# --------------------------
# create library
# --------------------------
add_library(arawn STATIC ${SOURCES} ${HEADERS} ${SHADER_TARGETS})
add_library(arawn::engine ALIAS arawn)

add_dependencies(arawn shaders)

target_compile_options(arawn
PRIVATE
	"-fdebug-prefix-map=${CMAKE_SOURCE_DIR}/="
	"-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/="
	--embed-dir=${CMAKE_CURRENT_BINARY_DIR}/import
)

target_link_libraries(arawn 
PRIVATE
	glfw
	GPUOpen::VulkanMemoryAllocator
	Vulkan::Vulkan
#	 stb::stb
#	 assimp::assimp
#	 meshoptimizer
)

target_include_directories(arawn 
PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/inc
	${glm_SOURCE_DIR}
PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/import
)

# --------------------------
# library options
# --------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(DEFAULT_ARAWN_DEBUG ON)
else()
	set(DEFAULT_ARAWN_DEBUG OFF)
endif()

option(ARAWN_DEBUG "enables aditional validation layers" ${DEFAULT_ARAWN_DEBUG})
option(ARAWN_LOG_VERBOSE "Enable verbose logging" OFF)
option(ARAWN_LOG_INFO "Enable debug logging" OFF)
option(ARAWN_LOG_WARNING "Enable warning logging" OFF)
option(ARAWN_LOG_ERROR "Enable error logging" OFF)

target_compile_definitions(arawn
PRIVATE
	ARAWN_VERSION_VARIANT=${ARAWN_VERSION_VARIANT}
	ARAWN_VERSION_MAJOR=${ARAWN_VERSION_MAJOR}
	ARAWN_VERSION_MINOR=${ARAWN_VERSION_MINOR}
	ARAWN_VERSION_PATCH=${ARAWN_VERSION_PATCH}
	$<$<BOOL:${ARAWN_DEBUG}>:ARAWN_DEBUG>
	$<$<BOOL:${ARAWN_LOG_VERBOSE}>:ARAWN_LOG_VERBOSE>
	$<$<BOOL:${ARAWN_LOG_INFO}>:ARAWN_LOG_INFO>
	$<$<BOOL:${ARAWN_LOG_WARNING}>:ARAWN_LOG_WARNING>
	$<$<BOOL:${ARAWN_LOG_ERROR}>:ARAWN_LOG_ERROR>
)