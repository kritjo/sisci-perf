#include <stdio.h>
#include <stdbool.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "printfo.h"
#include "error_util.h"

#define NO_FLAGS 0

void print_all(unsigned int adapter_no) {
    print_local_adapter(adapter_no);
    print_adapter_card_type(adapter_no);
    print_dma_availability(adapter_no);
}

void print_local_adapter(unsigned int adapter_no) {
	unsigned int local_node_id;
	sci_query_adapter_t query;
	sci_error_t error;

	query.subcommand = SCI_Q_ADAPTER_NODEID;
	query.localAdapterNo = adapter_no;
	query.data = &local_node_id;

	SCIQuery(SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIQuery", false);
	
	printf("Adapter %u has node id %u\n", adapter_no, local_node_id);
}

void print_adapter_card_type(unsigned int adapter_no) {
	unsigned int card_type;
	sci_query_adapter_t query;
	sci_error_t error;	

	query.subcommand = SCI_Q_ADAPTER_CARD_TYPE;
	query.localAdapterNo = adapter_no;
	query.data = &card_type;

	SCIQuery(SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIQuery", false);

	printf("Adapter %u has card type %s\n", adapter_no, SCIGetAdapterTypeString(card_type)); 
}

static void print_dma_capabilities(sci_dma_capabilities_t *dma_caps) {
    printf("        Number of Channels: %u\n", dma_caps->num_channels);
    printf("        max_transfer_size: %u\n", dma_caps->max_transfer_size);
    printf("        max_vector_length: %u\n", dma_caps->max_vector_length);
    printf("        min_align_bytes: %u\n", dma_caps->min_align_bytes);
    printf("        suggested_align_bytes: %u\n", dma_caps->suggested_align_bytes);
}

void print_dma_availability(unsigned int adapter_no) {
    sci_query_dma_t query;
    sci_error_t error;
    unsigned int flags[3] = {SCI_FLAG_DMA_ADAPTER, SCI_FLAG_DMA_SYSDMA, SCI_FLAG_DMA_GLOBAL};
    unsigned int avail[3];
    sci_dma_capabilities_t dma_caps[3];

    query.localAdapterNo = adapter_no;

    for (int i = 0; i < 3; i++) {
        query.data = &avail[i];
        query.subcommand = SCI_Q_DMA_AVAILABLE;
        SCIQuery(SCI_Q_DMA, &query, flags[i], &error);
        print_sisci_error(&error, "SCIQuery", false);

        query.data = &dma_caps[i];
        query.subcommand = SCI_Q_DMA_CAPABILITIES;
        SCIQuery(SCI_Q_DMA, &query, flags[i], &error);
        print_sisci_error(&error, "SCIQuery", false);
    }

    printf("Adapter %u has DMA availability:\n", adapter_no);
    printf("    Adapter: %s\n", avail[0] ? "Yes" : "No");
    if (avail[0]) print_dma_capabilities(&dma_caps[0]);
    printf("    SysDMA: %s\n", avail[1] ? "Yes" : "No");
    if (avail[1]) print_dma_capabilities(&dma_caps[1]);
    printf("    Global: %s\n", avail[2] ? "Yes" : "No");
    if (avail[2]) print_dma_capabilities(&dma_caps[2]);
}

