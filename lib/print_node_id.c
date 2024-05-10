#include <sisci_api.h>
#include "sisci_glob_defs.h"

unsigned int get_node_id() {
    unsigned int local_node_id;
    sci_query_adapter_t query;
    sci_error_t error;

    query.subcommand = SCI_Q_ADAPTER_NODEID;
    query.localAdapterNo = ADAPTER_NO;
    query.data = &local_node_id;

    SCIQuery(SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Error getting node id: %s\n", SCIGetErrorString(error));
        return 0;
    }

    return local_node_id;
}