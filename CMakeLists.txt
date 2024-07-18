cmake_minimum_required(VERSION 3.23)
project(HierarchicalPageTables)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(HierarchicalPageTables
        MemoryConstants.h
        MemoryConstants_test1.h
        MemoryConstants_test2.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        SimpleTest.cpp
        test1_write_read_all_virtual_memory.cpp
        test2_write_one_page_twice_and_read.cpp
        VirtualMemory.cpp
        VirtualMemory.h)
