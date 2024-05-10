#include <stdio.h>
#include <stdlib.h>
#include <dis_types.h>
#include <sisci_api.h>
#include "sisci_glob_defs.h"

int main(int argc, char *argv[]) {
    unsigned int nodeId;
    dis_nodeId_list_t nodelist;
    dis_virt_adapter_t adapter_type;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
    }

    SEOE(SCIInitialize, NO_FLAGS);


    SEOE(SCIGetNodeIdByAdapterName, argv[1], &nodelist, &adapter_type, NO_FLAGS);

    if (nodelist[0] != 0) {
        nodeId = nodelist[0];
    } else {
        fprintf(stderr, "No node with hostname %s found\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (nodelist[1] != 0) {
        fprintf(stderr, "More than one node with hostname %s found\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    printf("%u\n", nodeId);

    SCITerminate();
}