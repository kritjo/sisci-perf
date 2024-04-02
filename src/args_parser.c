#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#include "args_parser.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "error_util.h"
#include "sisci_glob_defs.h"

#define ARG_INIT(num_ptr) \
            do { if (*num_ptr != UNINITIALIZED_ARG) print_usage(argv[0]); } while (0)

#define ARG_PARSE(num_ptr, arg, argc, argv, parse_func, print_usage) \
            do { if (*num_ptr != UNINITIALIZED_ARG) print_usage(argv[0]); \
            parse_func(arg, argv, num_ptr); \
            if (*num_ptr == UNINITIALIZED_ARG) print_usage(argv[0]); } while (0)

static void parse_uint(int *arg, char *argv[], unsigned int *receiver_node_id) {
    long strtol_tmp;
    char *endptr;

    errno = 0;

    strtol_tmp = strtol(argv[++*arg], &endptr, 10);
    if (errno != 0) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (strtol_tmp > UINT_MAX || strtol_tmp < 0) return;
    if (*endptr != '\0') return;

    *receiver_node_id = (unsigned int) strtol_tmp;
}

static void parse_an(int *arg, char *argv[], unsigned int *receiver_node_id) {
    dis_virt_adapter_t adapter_type;
    sci_error_t error;
    dis_nodeId_list_t nodelist;

    SCIGetNodeIdByAdapterName(argv[++*arg], &nodelist, &adapter_type, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetNodeIdByAdapterName", true);

    if (nodelist[0] != 0) {
        printf("Found node with id %u matching adapter name\n", nodelist[0]);
        *receiver_node_id = nodelist[0];
    } else {
        printf("No matching adapters found matching %s\n", argv[*arg]);
        return;
    }

    if (nodelist[1] != 0) printf("    (multiple adapters found)\n");
}

int parse_id_args(int argc, char *argv[], unsigned int *rnid, unsigned int *use_local_addr, unsigned int *req_chnl, void (*print_usage)(char *)) {
    ARG_INIT(rnid);
    ARG_INIT(use_local_addr);
    ARG_INIT(req_chnl);
    int arg;

    for (arg = 1; arg < argc; arg++) {
        DEBUG_PRINT("Parsing argument: %s\n", argv[arg]);
        if (strcmp(argv[arg], "-nid") == 0) {
            ARG_PARSE(rnid, &arg, argc, argv, parse_uint, print_usage);
        }
        else if (strcmp(argv[arg], "-an") == 0) {
            ARG_PARSE(rnid, &arg, argc, argv, parse_an, print_usage);
        }
        else if (strcmp(argv[arg], "--use-local-addr") == 0) {
            *use_local_addr = 1;
        }
        else if (strcmp(argv[arg], "--request-channel") == 0) {
            *req_chnl = 1;
        }
        else if (arg == argc-1) {
            arg++;
            break;
        }
        else {
            printf("Unknown argument: %s\n", argv[arg]);
            print_usage(argv[0]);
        }
    }
    return arg;
}