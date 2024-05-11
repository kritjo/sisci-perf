add_executable(initiator_main
        initiator_main.c
        initiator_main.h

        simple_pio.c
        simple_pio.h
        simple_dma.c
        simple_dma.h
        scale_up_pio.c
        scale_up_pio.h
        scale_out_pio.c
        scale_out_pio.h
        var_size_pio.c
        var_size_pio.h)
target_link_libraries(initiator_main PRIVATE SISCIGlobDefs)
target_link_libraries(initiator_main PRIVATE sisci)