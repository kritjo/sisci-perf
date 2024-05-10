add_library(SISCIGlobDefs print_node_id.c print_node_id.h protocol.h sisci_glob_defs.h
        block_for_termination_signal.c block_for_termination_signal.h
        timer_controlled_variable.c timer_controlled_variable.h)
target_include_directories(SISCIGlobDefs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
