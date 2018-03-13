#ifndef UIP_INTERFACE_H
#define UIP_INTERFACE_H

#include "locking.h"
#include "enc28j60.h"

extern lock_t uip_buf_lock;

void uip_setup(void);
void uip_loop(void);
void uip_handle_new_packet(ENC_HandleTypeDef *handle);

#endif /* end of include guard: UIP_INTERFACE_H */
