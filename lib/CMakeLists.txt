add_library(SISCIGlobDefs print_node_id.c print_node_id.h protocol.h sisci_glob_defs.h)
target_include_directories(SISCIGlobDefs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
