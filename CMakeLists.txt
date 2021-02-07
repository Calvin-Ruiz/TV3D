cmake_minimum_required(VERSION 3.10)
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} )

project(mini_projet.out)

file(GLOB all_SRCS
    "${PROJECT_SOURCE_DIR}/*.cpp"
    )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

SET(OpenGL_GL_PREFERENCE "GLVND")
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)

SET(CMAKE_C_FLAGS "-g3")
SET(CMAKE_CXX_FLAGS "-g3 -Wextra -Wall -Wno-sign-compare -Wno-unused-but-set-variable")

add_executable(mini_projet.out ${all_SRCS})
target_link_libraries(mini_projet.out ${OPENGL_LIBRARY} ${GLFW_LIBRARY} ${GLEW_LIBRARY} ${GLEW_LIBRARIES} glfw X11 pthread)
