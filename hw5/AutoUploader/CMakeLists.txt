cmake_minimum_required(VERSION 3.7)
project(AutoUploader)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES client.cpp Executor.h)
add_executable(AutoUploader ${SOURCE_FILES})
add_executable(Server server.cpp)
