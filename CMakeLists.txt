cmake_minimum_required(VERSION 3.5)

project(sisci-perf)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -I/opt/DIS/include/ -I/opt/DIS/include/dis/ -g -D_REENTRANT")

# Set the path to the sisci library
set(SISCI_LIB_DIR /opt/DIS/lib64)

# Add the directory to the library search path
link_directories(${SISCI_LIB_DIR})

# Add the sisci library as an imported target
add_library(__sisci SHARED IMPORTED)
set_target_properties(__sisci PROPERTIES IMPORTED_LOCATION ${SISCI_LIB_DIR}/libsisci.so)

# Create an interface library to propagate the include directory and linker flags
add_library(sisci INTERFACE)
target_include_directories(sisci INTERFACE /opt/DIS/include)
target_link_libraries(sisci INTERFACE __sisci)

add_subdirectory(lib)
add_subdirectory(initiator)
add_subdirectory(peer)
add_subdirectory(tools)
