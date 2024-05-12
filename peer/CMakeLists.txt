add_executable(peer_main
        peer_main.c

        ping_pong_thread.c
        ping_pong_thread.h
)
target_link_libraries(peer_main PRIVATE SISCIGlobDefs)
target_link_libraries(peer_main PRIVATE sisci)
