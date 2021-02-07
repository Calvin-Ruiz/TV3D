cmake_minimum_required(VERSION 3.10)
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} )

project(mini_projet.out)

file(GLOB all_SRCS
    "${PROJECT_SOURCE_DIR}/*.cpp"
    )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

FIND_PACKAGE(Vulkan REQUIRED)

SET(CMAKE_C_FLAGS "-g3")
SET(CMAKE_CXX_FLAGS "-g3 -Wextra -Wall -Wno-sign-compare -Wno-unused-but-set-variable -Wno-pointer-arith")

add_executable(mini_projet.out ${all_SRCS})
target_link_libraries(mini_projet.out  ${Vulkan_LIBRARIES} ${GLFW_LIBRARY} glfw X11 pthread)
