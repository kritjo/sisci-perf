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
        var_size_pio.h
        var_size_dma.c
        var_size_dma.h
        dma_vec.c
        dma_vec.h
        simple_interrupt.c
        simple_interrupt.h
        simple_data_interrupt.c
        simple_data_interrupt.h
        ping_pong_pio.c
        ping_pong_pio.h
        broadcast_pio.c
        broadcast_pio.h
        broadcast_dma.c
        broadcast_dma.h
        var_size_segments.c
        var_size_segments.h
        scale_up_out_pio.c
        scale_up_out_pio.h
        interleaved_dmas.c
        interleaved_dmas.h
)
target_link_libraries(initiator_main PRIVATE SISCIGlobDefs)
target_link_libraries(initiator_main PRIVATE sisci)