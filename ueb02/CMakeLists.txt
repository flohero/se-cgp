cmake_minimum_required(VERSION 3.19)
project(ueb02)

set(CMAKE_CXX_STANDARD 14)

add_executable(ueb02
        main.cpp)

find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
target_link_libraries(ueb02 GLEW::GLEW GLUT::GLUT OpenGL::OpenGL OpenGL::GLU)