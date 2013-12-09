/*
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
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>
#include "string.h"

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#include "udp-server.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MAX_PAYLOAD_LEN 40

static struct uip_udp_conn *server_conn;

PROCESS(udp_server_process, "UDP server process");
//AUTOSTART_PROCESSES(&resolv_process,&udp_server_process);

static int seq_id;

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{  
	char udp_server_buf[MAX_PAYLOAD_LEN];
	
	memset(udp_server_buf, 0, MAX_PAYLOAD_LEN);

	if(uip_newdata()) {
		
		PRINTF("Server received packet from ", (char *)uip_appdata);
		PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
		PRINTF(" \n");

		uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
		PRINTF("Message length [%u]: ", ((uint8_t*)uip_appdata)[0]);
		
		PRINTF("Responding with current sequence number: [%u].", seq_id);
				
		udp_server_buf[0] = 1;
		udp_server_buf[1] = seq_id; 
		/* Increment sequence number. */
		seq_id++;
	
		PRINTF("Response is now sent to: ");
		PRINT6ADDR(&(server_conn->ripaddr));
		PRINTF(" \n");

		/* Send the response to the remote host. */
		uip_udp_packet_send(server_conn, udp_server_buf, MAX_PAYLOAD_LEN);
		/* Restore server connection to allow data from any node */
		memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
  }
}
/*---------------------------------------------------------------------------*/
static void print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF(" \n");
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
#if UIP_CONF_ROUTER
	uip_ipaddr_t ipaddr;
#endif /* UIP_CONF_ROUTER */

  PROCESS_BEGIN();
  
  /* Initialize sequence id. */
  seq_id = 0;
  
  /* Start resolv_process [removed from the autostart macro]. */
  process_start(&resolv_process, NULL);
  
  PRINTF("UDP server started\n");

#if RESOLV_CONF_SUPPORTS_MDNS
	PRINTF("UDP_SERVER: Resolve host-name: contiki-udp-server.\n");
	resolv_set_hostname("contiki-udp-server");
#endif

#if UIP_CONF_ROUTER
	PRINTF("UDP_SERVER: Router_Conf.\n");
	PRINTF("UDP_SERVER: MAC Address: %02x:%02x:%02x:%02x:%02x:%02x.\n",
		uip_lladdr.addr[0],
		uip_lladdr.addr[1],
		uip_lladdr.addr[2],
		uip_lladdr.addr[3],
		uip_lladdr.addr[4],
		uip_lladdr.addr[5]);
	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif /* UIP_CONF_ROUTER */

	print_local_addresses();

	server_conn = udp_new(NULL, UIP_HTONS(3001), NULL);
	
	if (server_conn == NULL) {
		PRINTF("UDP_SERVER_ERROR: Connection could not be created.\n");
	}
	/* Bind UDP Connection to local port. */		
	udp_bind(server_conn, UIP_HTONS(3000));

	PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(server_conn->lport), UIP_HTONS(server_conn->rport));
	
	while(1) {
		PROCESS_YIELD();
		if(ev == tcpip_event) {
		
			tcpip_handler();
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
