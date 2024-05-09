#include <sisci_api.h>
#include <sisci_error.h>
#include <sisci_types.h>
#include <stdio.h>
#include <stdlib.h>
#include "sisci_glob_defs.h"


int main(void) {
    unsigned int local_node_id;
    sci_query_adapter_t query;

    query.subcommand = SCI_Q_ADAPTER_NODEID;
    query.localAdapterNo = ADAPTER_NO;
    query.data = &local_node_id;

    SEOE(SCIQuery, SCI_Q_ADAPTER, &query, NO_FLAGS);

    printf("%d\n", local_node_id);
}