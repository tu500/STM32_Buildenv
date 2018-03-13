#ifndef HARDWARE_H
#define HARDWARE_H

#include "locking.h"

typedef void (*spi_largebuf_callback_t)(void);

extern lock_t spi_lock;
extern spi_largebuf_callback_t spi_largebuf_callback; // bound to spi_lock

void enc28j60_init_spi(void);

#endif /* end of include guard: HARDWARE_H */
