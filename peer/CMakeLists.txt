add_executable(peer_main peer_main.c)
target_link_libraries(peer_main PRIVATE SISCIGlobDefs)
target_link_libraries(peer_main PRIVATE sisci)
