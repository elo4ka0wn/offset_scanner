cmake_minimum_required(VERSION 3.16)
project(offset_scanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(offset_scanner
    src/main.cpp
    src/Vector3.h
    src/ProcessUtils.cpp
    src/ProcessUtils.h
    src/ProcessMemoryReader.cpp
    src/ProcessMemoryReader.h
    src/OffsetScanner.cpp
    src/OffsetScanner.h
)

if(UNIX)
    target_link_libraries(offset_scanner PRIVATE pthread)
endif()
