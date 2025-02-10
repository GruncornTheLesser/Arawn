cmake_minimum_required(VERSION 3.16)

include(FetchContent)

project(ARAWN VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(ARAWN ${SOURCES})

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

FetchContent_Declare(entt GIT_REPOSITORY https://github.com/skypjack/entt GIT_TAG master)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm GIT_TAG master)

FetchContent_MakeAvailable(glm entt)

include_directories(${CMAKE_SOURCE_DIR}/inc)

target_include_directories(ARAWN PRIVATE ${glm_SOURCE_DIR} ${entt_SOURCE_DIR}/single_include)
target_link_libraries(ARAWN PRIVATE glfw vulkan)

file(GLOB_RECURSE SHADERS "res/*.vert" "res/*.frag" "res/*.comp" "res/*.geom")

# Shader compilation logic
set(RESOURCE_DIR "${PROJECT_SOURCE_DIR}/res")

foreach(SHADER ${SHADERS})
    file(RELATIVE_PATH SHADER_DIR ${RESOURCE_DIR} ${SHADER})
    set(TARGET "${RESOURCE_DIR}/import/${SHADER_DIR}.spv")
    get_filename_component(TARGET_DIR ${TARGET} DIRECTORY)
    
    string(REPLACE "/" "_" SHADER_NAME ${SHADER_DIR})
    string(REPLACE "." "_" SHADER_NAME ${SHADER_NAME})
    
    message("${SHADER_NAME}")

    add_custom_command(
        OUTPUT ${TARGET}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGET_DIR}
        COMMAND glslc ${SHADER} -o ${TARGET}
        DEPENDS ${SHADER}
        COMMENT "compiling shader: ${SHADER_DIR}"
        VERBATIM
    )
    
    add_custom_target(shader_${SHADER_NAME} DEPENDS ${TARGET})
    add_dependencies(ARAWN shader_${SHADER_NAME})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SHADERS})

add_dependencies(ARAWN compile_shaders)