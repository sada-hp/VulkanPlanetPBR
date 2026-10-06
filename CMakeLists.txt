include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

file(GLOB_RECURSE CODE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

add_library(source STATIC ${CODE_SOURCES})

add_compile_definitions(DEBUG=$<CONFIG:Debug>)
target_compile_definitions(source PUBLIC EXPORT_API)
target_precompile_headers(source PUBLIC source/pch.hpp)

set_target_properties(source PROPERTIES  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${COMPILE_PATH})
set_target_properties(source PROPERTIES  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${COMPILE_PATH})

target_link_libraries(source PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libraries/glfw3.lib)
target_link_libraries(source PUBLIC Vulkan::Vulkan Vulkan::shaderc_combined)
target_include_directories(source PUBLIC ${Vulkan_INCLUDE_DIRS})