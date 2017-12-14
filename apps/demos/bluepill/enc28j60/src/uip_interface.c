#include <stdio.h>

#include "uip.h"
#include "uip_arp.h"
#include "enc28j60.h"
#include "timer.h"

static struct timer periodic_timer, arp_timer;

extern uint8_t MacAddress[6];
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

extern ENC_HandleTypeDef henc;
static void network_device_send(void)
{
  printf("Transmitting packet, %d\r\n", uip_len);
  henc.transmitLength = uip_len;
  ENC_RestoreTXBuffer(&henc, uip_len);
  ENC_WriteBuffer(uip_buf, uip_len);
  ENC_Transmit(&henc);
}

void uip_loop(void)
{
  size_t i;

  if(timer_expired(&periodic_timer)) {
    timer_reset(&periodic_timer);
    for(i = 0; i < UIP_CONNS; i++) {
      uip_periodic(i);
      /* If the above function invocation resulted in data that
	 should be sent out on the network, the global variable
	 uip_len is set to a value > 0. */
      if(uip_len > 0) {
	uip_arp_out();
	network_device_send();
      }
    }

#if UIP_UDP
    for(i = 0; i < UIP_UDP_CONNS; i++) {
      uip_udp_periodic(i);
      /* If the above function invocation resulted in data that
	 should be sent out on the network, the global variable
	 uip_len is set to a value > 0. */
      if(uip_len > 0) {
	uip_arp_out();
	network_device_send();
      }
    }
#endif /* UIP_UDP */

    /* Call the ARP timer function every 10 seconds. */
    if(timer_expired(&arp_timer)) {
      timer_reset(&arp_timer);
      uip_arp_timer();
    }
  }
}

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
void uip_handle_new_packet(ENC_HandleTypeDef *handle)
{
  uip_len = handle->RxFrameInfos.length;

  if(BUF->type == htons(UIP_ETHTYPE_IP)) {
  /* printf("New packet IP\r\n"); */
    uip_arp_ipin();
    uip_input();
    /* If the above function invocation resulted in data that
       should be sent out on the network, the global variable
       uip_len is set to a value > 0. */
    if(uip_len > 0) {
      uip_arp_out();
      network_device_send();
    }
  } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
  /* printf("New packet ARP\r\n"); */
    uip_arp_arpin();
    /* If the above function invocation resulted in data that
       should be sent out on the network, the global variable
       uip_len is set to a value > 0. */
    if(uip_len > 0) {
      network_device_send();
    }
  }
  else
  {printf("Unknown packet\r\n");}
}
