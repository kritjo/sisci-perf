add_library(SISCIGlobDefs print_node_id.c print_node_id.h protocol.h sisci_glob_defs.h
        block_for_termination_signal.c block_for_termination_signal.h
        common_read_write_functions.c common_read_write_functions.h)
target_include_directories(SISCIGlobDefs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
