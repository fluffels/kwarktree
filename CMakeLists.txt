cmake_minimum_required (VERSION 3.0)
set (CMAKE_CXX_STANDARD 17)

project (kwarktree)

find_package(Vulkan REQUIRED)

include_directories (${CMAKE_HOME_DIRECTORY}/src)
include_directories (${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Vulkan_INCLUDE_DIRS})
add_executable (
    main
    WIN32
    lib/SPIRV-Reflect/spirv_reflect.c
    src/Main.cpp
)
target_link_libraries(
    main
    ${Vulkan_LIBRARIES}
)
