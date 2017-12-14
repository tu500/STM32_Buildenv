#include <stdint.h>

#define UIP_CONF_MAX_CONNECTIONS 1
#define UIP_CONF_MAX_LISTENPORTS 0
#define UIP_CONF_BUFFER_SIZE 1000

#define UIP_CONF_UDP 0

//#define UIP_CONF_RECEIVE_WINDOW
//#define UIP_CONF_ARPTAB_SIZE

//#define UIP_CONF_LOGGING
//#define UIP_CONF_STATISTICS

void do_nothing(void);

//#define UIP_APPCALL do_nothing
#define UIP_APPCALL   if (uip_conn->appstate.callback) uip_conn->appstate.callback
typedef struct
{
  void (*callback)(void);
  void *custom_data;
} uip_tcp_appstate_t;
typedef void* uip_udp_appstate_t;

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint16_t uip_stats_t;
