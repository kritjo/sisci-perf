add_executable(initiator_main
        simple_pio.c
        simple_pio.h
        initiator_main.c
        simple_dma.c
        simple_dma.h)
target_link_libraries(initiator_main PRIVATE SISCIGlobDefs)
target_link_libraries(initiator_main PRIVATE sisci)