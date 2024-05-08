cmake_minimum_required(VERSION 3.5)

project(sisci-perf)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -I/opt/DIS/include/ -I/opt/DIS/include/dis/ -g -D_REENTRANT")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/DIS/lib64/ -lsisci")

add_subdirectory(lib)
add_subdirectory(initiator)
add_subdirectory(peer)
add_subdirectory(tools)
