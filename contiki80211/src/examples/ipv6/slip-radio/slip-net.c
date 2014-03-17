/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/uip.h"
#include "net/packetbuf.h"
#include "dev/slip.h"
#include <stdio.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#ifdef WITH_AR9170_WIFI_SUPPORT
/* For the driver access to the original MAC address. */
#include "uip_arp.h"
#include "ieee80211_ibss.h"
#endif

/* Debug flag for the slip 80211 network module. */
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG	0
#ifdef DEBUG
#include "uip-debug.h"
#endif
/*---------------------------------------------------------------------------*/
void
slipnet_init(void)
{
}
/*---------------------------------------------------------------------------*/
#ifdef WITH_AR9170_WIFI_SUPPORT
void slipnet_init_80211(void)
{
	/* Initialize the slip_net driver module. Just
	 * sets the Ethernet address for the ARP code. 
	 */
	uip_lladdr_t mac_80211_addr;
	if (unique_vif->addr == NULL) {
		/* Report an error and do not initialize. */
		PRINTF("SLIP-NET ERROR: Cannot find MAC addres.\n");
		return;
	}
	memcpy(mac_80211_addr.addr, unique_vif->addr, UIP_LLADDR_LEN);
	uip_setethaddr(mac_80211_addr);
	#if SLIP_NET_DEBUG
	PRINTF("SLIP-NET: Copying MAC address to UIP ARP: %02x:%02x:%02x:%02x:%02x:%02x.\n",
			uip_lladdr.addr[0],
			uip_lladdr.addr[1],
			uip_lladdr.addr[2],
			uip_lladdr.addr[3],
			uip_lladdr.addr[4],
			uip_lladdr.addr[5]);
	#endif	
}
#endif
/*---------------------------------------------------------------------------*/
void
slip_send_packet(const uint8_t *ptr, int len)
{
	uint16_t i;
	uint8_t c;

	slip_arch_writeb(SLIP_END);
	for(i = 0; i < len; ++i) {
		c = *ptr++;
		if(c == SLIP_END) {
			slip_arch_writeb(SLIP_ESC);
			c = SLIP_ESC_END;
		} else if(c == SLIP_ESC) {
			slip_arch_writeb(SLIP_ESC);
			c = SLIP_ESC_ESC;
		}
		slip_arch_writeb(c);
	}
	slip_arch_writeb(SLIP_END);
}
/*---------------------------------------------------------------------------*/
void
slipnet_input(void)
{
  int i;
  /* radio should be configured for filtering so this should be simple */
  /* this should be sent over SLIP! */
  /* so just copy into uip-but and send!!! */
  /* Format: !R<data> ? */
  uip_len = packetbuf_datalen();
  i = packetbuf_copyto(uip_buf);

  if(DEBUG) {
    printf("Slipnet got input of len: %d, copied: %d\n",
           packetbuf_datalen(), i);

    for(i = 0; i < uip_len; i++) {
      printf("%02x", (unsigned char) uip_buf[i]);
      if((i & 15) == 15) printf(" \n");
      else if((i & 7) == 7) printf(" ");
    }
    printf(" \n");
  }

  /* printf("SUT: %u\n", uip_len); */
  slip_send_packet(uip_buf, uip_len);
}
/*---------------------------------------------------------------------------*/
void
slipnet_input_80211(void)
{
	PRINTF("SLIPNET: input_80211 [%u]:",packetbuf_datalen());
	int k;
	uint8_t* test = packetbuf_dataptr();
	for (k=0; k<packetbuf_datalen(); k++) {
		PRINTF("%02x",test[k]);
	}
	printf(" \n");
	/* Create the SLIP frame based on the incoming data.
	 * The packet is directly put inside the uip-buffer;
	 * consider doing something smarter; but the create
	 * gets no arguments....
	 */
	int i = NETSTACK_FRAMER.create();	
	if (i < 0) {
		/* Packet is dropped - not forwarded to the border router. */
		PRINTF("SLIPnet80211 ERROR; Creation failed.\n");
		return;
	}
	
	if (DEBUG) {
		/* Print-out the frame contents. */
		printf("Slipnet80211 got input of len: %d, copied: %d\n", packetbuf_datalen(), i);

		for (i = 0; i < uip_len; i++) {
			
			printf("%02x", (unsigned char) uip_buf[i]);
			if ((i & 15) == 15) 
				printf(" \n");
			else if ((i & 7) == 7) 
				printf(" ");
		}
		printf(" \n");
	}
	/* printf("SUT: %u\n", uip_len); */
	slip_send_packet(uip_buf, uip_len);
}	
/*---------------------------------------------------------------------------*/
const struct network_driver slipnet_driver = {
	#ifdef WITH_AR9170_WIFI_SUPPORT
	"slipnet80211",
	slipnet_init_80211,
	slipnet_input_80211,
	#else
	"slipnet",
	slipnet_init,
	slipnet_input
	#endif
};
/*---------------------------------------------------------------------------*/