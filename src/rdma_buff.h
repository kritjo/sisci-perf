#ifndef KRITJO_SISCI_RDMA_BUFF
#define KRITJO_SISCI_RDMA_BUFF
typedef struct {
    unsigned char done   :1;
    unsigned char        :3;

    char word[3];
} rdma_buff_t;
#endif
