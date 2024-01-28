#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0

#define ADAPTER_NO

#define SENDER_NODE_ID
#define SENDER_SEG_ID
#define SENDER_SEG_SIZE

#define RECEIVER_SEG_ID
#define RECEIVER_NODE_ID
#define RECEIVER_SEG_SIZE


int main(void) {
    sci_error_t error;

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true); 
    printf("SCI initialization OK!\n");

    // Core program here

    SCITerminate();
    printf("SCI termination OK!\n");
    return EXIT_SUCCESS;
}
