#ifndef KRITJO_SISCI_RNID_UTIL
#define KRITJO_SISCI_RNID_UTIL

#define UNINITIALIZED_ARG -10

int parse_id_args(int argc, char *argv[], unsigned int *rnid, unsigned int *channel_id, void (*print_usage)(char *));

#endif //KRITJO_SISCI_RNID_UTIL
