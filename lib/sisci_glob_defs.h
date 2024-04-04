#ifndef KRITJOLIB_SISCI_GLOB_DEFS
#define KRITJOLIB_SISCI_GLOB_DEFS

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0
#define NO_OFFSET 0
#define NO_SUG_ADDR 0
#define ADAPTER_NO 0 // From /etc/dis/dishosts.conf
#define RECEIVER_SEG_ID 1
#define RECEIVER_SEG_SIZE 4096

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#endif //KRITJOLIB_SISCI_GLOB_DEFS
