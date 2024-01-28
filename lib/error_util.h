#ifndef KRITJOLIB_SISCI_ERROR_UTILITY
#define KRITJOLIB_SISCI_ERROR_UTILITY

#include "/opt/DIS/include/sisci_error.h"

void print_sisci_error(sci_error_t *error, char *func, int exit_on_failure);

#endif
