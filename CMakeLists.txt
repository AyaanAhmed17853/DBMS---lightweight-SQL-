cmake_minimum_required(VERSION 3.10)
project(lightweight_db)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define source files
set(SOURCES
    src/main.cpp
    src/Pager.cpp
    src/Table.cpp
    src/BPlusTree.cpp
    src/Parser.cpp
    src/Executor.cpp
)

# Add the executable
add_executable(lightweight_db ${SOURCES})

# Include directories
target_include_directories(lightweight_db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
