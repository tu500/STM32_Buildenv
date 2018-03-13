#include <stdio.h>

#include "uip.h"
#include "uip_arp.h"
#include "enc28j60.h"
#include "timer.h"

#include "locking.h"
#include "hardware.h"
#include "uip_interface.h"

/* #define DATA_DEBUG */

extern uint8_t MacAddress[6];
extern ENC_HandleTypeDef henc;

lock_t uip_buf_lock;

static struct timer periodic_timer, arp_timer;

void uip_setup(void)
{
  uip_ipaddr_t ipaddr;

  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  uip_init();

  /* uip_ipaddr(ipaddr, 192,168,0,2); */
  uip_ipaddr(ipaddr, 172,23,23,108);
  uip_sethostaddr(ipaddr);

  /* uip_ipaddr(ipaddr, 255,255,255,0); */
  uip_ipaddr(ipaddr, 255,255,254,0);
  uip_setnetmask(ipaddr);

  uip_setethaddr( (*((struct uip_eth_addr*) &MacAddress[0])) );
}

// implicitly gets a lock on the uip_buf
static void network_device_send(void)
{
  printf("Queuing packet, %d\r\n", uip_len);
#ifdef DATA_DEBUG
  printf("%d 000 ", HAL_GetTick());
  for (size_t i=0; i<uip_len; i++)
    printf("%02x", uip_buf[i]);
  printf("\r\n");
#endif

  henc.txState.queue_data = uip_buf;
  henc.txState.queue_length = uip_len;
}

void uip_loop(void)
{
  static uint16_t tcp_conn_counter = 0;
  static uint16_t udp_conn_counter = 0;

  if(timer_expired(&periodic_timer))
  {
    timer_reset(&periodic_timer);
    tcp_conn_counter = 0;
    udp_conn_counter = 0;
  }

  /* Call the ARP timer function every 10 seconds. */
  if(timer_expired(&arp_timer)) {
    timer_reset(&arp_timer);
    uip_arp_timer();
  }

  while (tcp_conn_counter < UIP_CONNS)
  {
    if (!lock_acquire(&uip_buf_lock))
      return;

    uip_periodic(tcp_conn_counter);
    tcp_conn_counter++;

    /* If the above function invocation resulted in data that
       should be sent out on the network, the global variable
       uip_len is set to a value > 0. */
    if(uip_len > 0)
    {
      uip_arp_out();
      network_device_send(); // pass uip_buf_lock
    }
    else
    {
      lock_release(&uip_buf_lock);
    }
  }

#if UIP_UDP
  while (udp_conn_counter < UIP_UDP_CONNS)
  {
    if (!lock_acquire(&uip_buf_lock))
      return;

    uip_udp_periodic(udp_conn_counter);
    udp_conn_counter++;

    /* If the above function invocation resulted in data that
       should be sent out on the network, the global variable
       uip_len is set to a value > 0. */
    if(uip_len > 0)
    {
      uip_arp_out();
      network_device_send(); // pass uip_buf_lock
    }
    else
    {
      lock_release(&uip_buf_lock);
    }
  }
#endif /* UIP_UDP */
}

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
// implicitly gets a lock on the uip_buf
void uip_handle_new_packet(ENC_HandleTypeDef *handle)
{
  uip_len = handle->rxState.length;

  printf("new packet RX: %d\r\n", uip_len);
#ifdef DATA_DEBUG
  printf("%d 000 ", HAL_GetTick());
  for (size_t i=0; i<uip_len; i++)
    printf("%02x", uip_buf[i]);
  printf("\r\n");
#endif

  if(BUF->type == htons(UIP_ETHTYPE_IP))
  {
    /* printf("New packet IP\r\n"); */
    uip_arp_ipin();
    uip_input();
    /* If the above function invocation resulted in data that
       should be sent out on the network, the global variable
       uip_len is set to a value > 0. */
    if(uip_len > 0)
    {
      uip_arp_out();
      network_device_send(); // implicitly pass uip_buf lock
    }
    else
    {
      lock_release(&uip_buf_lock);
    }
  }
  else if(BUF->type == htons(UIP_ETHTYPE_ARP))
  {
    /* printf("New packet ARP\r\n"); */
    uip_arp_arpin();
    /* If the above function invocation resulted in data that
       should be sent out on the network, the global variable
       uip_len is set to a value > 0. */
    if(uip_len > 0)
    {
      network_device_send(); // implicitly pass uip_buf lock
    }
    else
    {
      lock_release(&uip_buf_lock);
    }
  }
  else
  {
    printf("Unknown packet\r\n");
    lock_release(&uip_buf_lock);
  }
}
