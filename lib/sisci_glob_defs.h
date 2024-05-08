#ifndef SISCI_PERF_SISCI_GLOB_DEFS_H
#define SISCI_PERF_SISCI_GLOB_DEFS_H

#define ADAPTER_NO 0

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0
#define NO_OFFSET 0
#define NO_SUGGESTED_ADDRESS 0

// SISCI Exit On Error -- SEOE
#define SEOE(func, ...) \
do { \
    sci_error_t error; \
    func(__VA_ARGS__, &error); \
    if (error != SCI_ERR_OK) { \
        fprintf(stderr, "%s failed: %s\n", #func, SCIGetErrorString(error)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#endif //SISCI_PERF_SISCI_GLOB_DEFS_H
