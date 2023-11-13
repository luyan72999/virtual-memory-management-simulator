cmake_minimum_required(VERSION 3.26)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        main.cpp
        tlb.cpp
        process.cpp
        os.cpp
        page-table.cpp
)

add_executable(untitled ${SOURCE_FILES})
set_target_properties(untitled PROPERTIES OUTPUT_NAME a.out)
