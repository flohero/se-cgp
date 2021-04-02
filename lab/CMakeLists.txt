cmake_minimum_required(VERSION 3.17)
project(Lab)

set(CMAKE_CXX_STANDARD 14)

add_executable(Lab
        lab2.cpp)

find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
target_link_libraries(Lab GLEW::GLEW GLUT::GLUT OpenGL::OpenGL OpenGL::GLU)
