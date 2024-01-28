#include <stdio.h>
#include <stdlib.h>

#include "error_util.h"

void print_sisci_error(sci_error_t *error, char *func, int exit_on_failure) {
    if (*error == SCI_ERR_OK) return;
    
    fprintf(stderr, "[%s] SCI ERROR: ", func);
    switch (*error) {
		case SCI_ERR_BUSY:
			fprintf(stderr, "Some resources are busy\n");
			break;
		case SCI_ERR_FLAG_NOT_IMPLEMENTED:
			fprintf(stderr, "This flag option is not implemented\n");
			break;
		case SCI_ERR_ILLEGAL_FLAG:
			fprintf(stderr, "Illegal flag option\n");
			break;
		case SCI_ERR_NOSPC:
			fprintf(stderr, "Out of local resources\n");
			break;
		case SCI_ERR_API_NOSPC:
			fprintf(stderr, "Out of local API resources\n");
			break;
		case SCI_ERR_HW_NOSPC:
			fprintf(stderr, "Out of hardware resources\n");
			break;
		case SCI_ERR_NOT_IMPLEMENTED:
			fprintf(stderr, "The functionality is currently not implemented\n");
			break;
		case SCI_ERR_ILLEGAL_ADAPTERNO:
			fprintf(stderr, "Illegal adapter number\n");
			break;
		case SCI_ERR_NO_SUCH_ADAPTERNO:
			fprintf(stderr, "The specified adapter number can not be found - Check the configuration\n");
			break;
		case SCI_ERR_TIMEOUT:
			fprintf(stderr, "The operation timed out\n");
			break;
		case SCI_ERR_OUT_OF_RANGE:
			fprintf(stderr, "The specified variable if not within the legal range\n");
			break;
		case (0x90C  | SISCI_ERR_MASK):
			fprintf(stderr, "The specified interrupt number is not found || The specified segmentId is not found\n");
			break;
		case SCI_ERR_ILLEGAL_NODEID:
			fprintf(stderr, "Illegal nodeId - Check the configuration\n");
			break;
		case SCI_ERR_CONNECTION_REFUSED:
			fprintf(stderr, "Connection to remote node is refused\n");
			break;
		case SCI_ERR_SEGMENT_NOT_CONNECTED:
			fprintf(stderr, "No connection to the segment\n");
			break;
		case SCI_ERR_SIZE_ALIGNMENT:
			fprintf(stderr, "The specified size is not aligned\n");
			break;
		case SCI_ERR_OFFSET_ALIGNMENT:
			fprintf(stderr, "The specified offset is not aligned\n");
			break;
		case SCI_ERR_ILLEGAL_PARAMETER:
			fprintf(stderr, "Illegal function parameter\n");
			break;
		case SCI_ERR_MAX_ENTRIES:
			fprintf(stderr, "Maximum possible physical mapping is exceeded - Check the configuration\n");
			break;
		case SCI_ERR_SEGMENT_NOT_PREPARED:
			fprintf(stderr, "The segment is not prepared - Check documentation for SCIPrepareSegment()\n");
			break;
		case SCI_ERR_ILLEGAL_ADDRESS:
			fprintf(stderr, "Illegal address\n");
			break;
		case SCI_ERR_ILLEGAL_OPERATION:
			fprintf(stderr, "Illegal operation\n");
			break;
		case SCI_ERR_ILLEGAL_QUERY:
			fprintf(stderr, "Illegal query operation - Check the documentation for function SCIQuery()\n");
			break;
		case SCI_ERR_SEGMENTID_USED:
			fprintf(stderr, "This segmentId is alredy used - The segmentId must be unique\n");
			break;
		case SCI_ERR_SYSTEM:
			fprintf(stderr, "Could not get requested resources from the system (OS dependent)\n");
			break;
		case SCI_ERR_CANCELLED:
			fprintf(stderr, "The operation was cancelled\n");
			break;
		case SCI_ERR_NOT_CONNECTED:
			fprintf(stderr, "The host is not connected to the remote host\n");
			break;
		case SCI_ERR_NOT_AVAILABLE:
			fprintf(stderr, "The requested operation is not available\n");
			break;
		case SCI_ERR_INCONSISTENT_VERSIONS:
			fprintf(stderr, "Inconsistent driver versions on local host or remote host\n");
			break;
		case SCI_ERR_COND_INT_RACE_PROBLEM:
			fprintf(stderr, "Interrupt race problem detected\n");
			break;
		case SCI_ERR_OVERFLOW:
			fprintf(stderr, "Out of local resources\n");
			break;
		case SCI_ERR_NOT_INITIALIZED:
			fprintf(stderr, "The host is not initialized - Check the configuration\n");
			break;
		case SCI_ERR_ACCESS:
			fprintf(stderr, "No local or remote access for the requested operation\n");
			break;
		case SCI_ERR_NOT_SUPPORTED:
			fprintf(stderr, "The request is not supported\n");
			break;
		case SCI_ERR_DEPRECATED:
			fprintf(stderr, "This function or functionality is deprecated\n");
			break;
		case SCI_ERR_DMA_NOT_AVAILABLE:
			fprintf(stderr, "The requested DMA mode, or the required DMA mode for the operation, is not available\n");
			break;
		case SCI_ERR_DMA_DISABLED:
			fprintf(stderr, "The requested DMA mode, or the required DMA mode for the operation, is disabled\n");
			break;
		case SCI_ERR_NO_SUCH_NODEID:
			fprintf(stderr, "The specified nodeId could not be found\n");
			break;
		case SCI_ERR_NODE_NOT_RESPONDING:
			fprintf(stderr, "The specified node does not respond to the request\n");
			break;
		case SCI_ERR_NO_REMOTE_LINK_ACCESS:
			fprintf(stderr, "The remote link is not operational\n");
			break;
		case SCI_ERR_NO_LINK_ACCESS:
			fprintf(stderr, "The local link is not operational\n");
			break;
		case SCI_ERR_TRANSFER_FAILED:
			fprintf(stderr, "The transfer failed\n");
			break;
		case SCI_ERR_SEMAPHORE_COUNT_EXCEEDED:
			fprintf(stderr, "Semaphore counter is exceeded\n");
			break;
		case SCI_ERR_IRQL_ILLEGAL:
			fprintf(stderr, "Illegal interrupt line\n");
			break;
		case (0xB00  | SISCI_ERR_MASK):
			fprintf(stderr, "The remote host is busy || This error code is not used by SISCI API - Should not be documented\n");
			break;
		case SCI_ERR_LOCAL_BUSY:
			fprintf(stderr, "The local host is busy\n");
			break;
		case SCI_ERR_ALL_BUSY:
			fprintf(stderr, "The system is busy\n");
			break;
		case SCI_ERR_NO_SUCH_FDID:
			fprintf(stderr, "The specified fabric device identifier is not found\n");
			break;	
        default:
            fprintf(stderr, "UNDEFINED ERROR!");
            exit(EXIT_FAILURE);
    }
	if (exit_on_failure) exit(EXIT_FAILURE);
}

