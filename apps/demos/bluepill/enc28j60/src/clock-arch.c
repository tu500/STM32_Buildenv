#include <stm32f1xx.h>

#include "clock-arch.h"

clock_time_t clock_time(void)
{
  return HAL_GetTick();
}
