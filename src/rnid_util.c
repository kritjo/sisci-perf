#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#include "rnid_util.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "error_util.h"

bool parse_nid(char *arg, unsigned int *receiver_node_id) {
    long strtol_tmp;
    char *endptr;

    errno = 0;

    strtol_tmp = strtol(arg, &endptr, 10);
    if (errno != 0) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (strtol_tmp > UINT_MAX || strtol_tmp < 0) return false;
    if (*endptr != '\0') return false;

    *receiver_node_id = (unsigned int) strtol_tmp;
    return true;
}

bool parse_an(char *arg, unsigned int *receiver_node_id) {
    dis_virt_adapter_t adapter_type;
    sci_error_t error;
    dis_nodeId_list_t nodelist;

    SCIGetNodeIdByAdapterName(arg, &nodelist, &adapter_type, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetNodeIdByAdapterName", true);

    if (nodelist[0] != 0) {
        printf("Found node with id %u matching adapter name\n", nodelist[0]);
        *receiver_node_id = nodelist[0];
    } else {
        printf("No matching adapters found matching %s\n", arg);
        return false;
    }

    if (nodelist[1] != 0) printf("    (multiple adapters found)\n");
    return true;
}

int parse_rnid_args(int argc, char *argv[], unsigned int *receiver_node_id, void (*print_usage)(char *)) {
    bool rnid_set = false;
    int arg;

    for (arg = 0; arg < argc; arg++) {
        if (strcmp(argv[arg], "-nid") == 0) {
            if (rnid_set) print_usage(argv[0]);
            if (arg+1 == argc) print_usage(argv[0]);
            rnid_set = parse_nid(argv[arg+1], receiver_node_id);
            if (!rnid_set) print_usage(argv[0]);
        }
        else if (strcmp(argv[arg], "-an") == 0) {
            if (rnid_set) print_usage(argv[0]);
            if (arg+1 == argc) print_usage(argv[0]);
            rnid_set = parse_an(argv[arg+1], receiver_node_id);
            if (!rnid_set) print_usage(argv[0]);
        }
    }

    if (!rnid_set) print_usage(argv[0]);
    return arg;
}