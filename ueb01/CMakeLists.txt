cmake_minimum_required(VERSION 3.17)
project(ueb01)

set(CMAKE_CXX_STANDARD 14)

add_executable(ueb01
        S1910307103_Weingartshofer_01.cpp)

find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
target_link_libraries(ueb01 GLEW::GLEW GLUT::GLUT OpenGL::OpenGL OpenGL::GLU)
