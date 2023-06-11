cmake_minimum_required(VERSION 3.21)
project(ex4)

set(CMAKE_CXX_STANDARD 14)

add_executable(ex4 MemoryConstants.h PhysicalMemory.h PhysicalMemory.cpp SimpleTest.cpp VirtualMemory.h VirtualMemory.cpp)
