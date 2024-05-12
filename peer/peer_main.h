#ifndef SISCI_PERF_PEER_MAIN_H
#define SISCI_PERF_PEER_MAIN_H

static sci_callback_action_t order_callback(__attribute__((unused)) void *_arg,
                                            __attribute__((unused)) sci_local_data_interrupt_t _interrupt,
                                            void *data,
                                            unsigned int length,
                                            sci_error_t status);

static void delivery_notification(delivery_status_t status, unsigned int commandType, unsigned int deliveryType, unsigned int id);

static sci_callback_action_t handle_create_order(order_t *order);
static sci_callback_action_t handle_destroy_order(order_t *order);



#endif //SISCI_PERF_PEER_MAIN_H
