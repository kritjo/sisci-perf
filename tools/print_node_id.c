#include <sisci_api.h>
#include <sisci_error.h>
#include <stdio.h>
#include <stdlib.h>
#include "print_node_id.h"
#include "sisci_glob_defs.h"


int main(void) {
    SEOE(SCIInitialize, NO_FLAGS);

    printf("%d\n", get_node_id());

    SCITerminate();
}