/*
 * slip_framer.c
 *
 * Created: 10/17/2013 5:22:14 PM
 *  Author: Ioannis Glaropoulos
 */ 
#include "net/mac/framer.h"
#include "net/mac/frame802154.h"
#include "net/packetbuf.h"
#include <string.h>


/* Enable or disable printout-debugs. */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTADDR(_addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)_addr)[0], ((uint8_t *)_addr)[1], ((uint8_t *)_addr)[2], ((uint8_t *)_addr)[3], ((uint8_t *)_addr)[4], ((uint8_t *)_addr)[5], ((uint8_t *)_addr)[6], ((uint8_t *)_addr)[7])
#else
#define PRINTF(...)
#define PRINTADDR(_addr)
#endif
#include "uip.h"
#include "etherdevice.h"

/**  \brief The 16-bit identifier of the PAN on which the device is
 *   sending to.  If this value is 0xffff, the device is not
 *   associated.
 */
static const uint16_t mac_dst_pan_id = IEEE802154_PANID;

/**  \brief The 16-bit identifier of the PAN on which the device is
 *   operating.  If this value is 0xffff, the device is not
 *   associated.
 */
static const uint16_t mac_src_pan_id = IEEE802154_PANID;
/*---------------------------------------------------------------------------*/
static int create_80211(void) {
	  
	int i;
	uint16_t _offset;
	/* The underlying 802.11 driver is delivering the 
	 * raw IPv6 packet to us. By using the additional
	 * packet information in the packet buffer, we'll
	 * update the packet header, so the border router
	 * is able to extract all the necessary info for
	 * the received packet. This action is equivalent
	 * to serialization of the input.
	 */
	_offset = 0;
	/* First, we copy the destination and the source
	 * [Rime-8byte] addresses of the incoming packet
	 */
	rimeaddr_t* dst_mac_address = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
	rimeaddr_t* src_mac_address = packetbuf_addr(PACKETBUF_ADDR_SENDER);
	
	if (dst_mac_address == NULL || src_mac_address == NULL) {
		/* Packet should not be forwarded upwards. */
		PRINTF("SLIP-NET ERROR: Addresses of incoming packet are NULL.\n");
		return -1;
	}
	memcpy(uip_buf, dst_mac_address, RIMEADDR_SIZE);
	_offset += RIMEADDR_SIZE;
	memcpy(uip_buf + _offset,src_mac_address, RIMEADDR_SIZE);
	_offset += RIMEADDR_SIZE;
	
	/* Next we extract the PAN ID and attach it 
	 * to the header we have constructed at the
	 * front of the UIP header. Since the 80211
	 * has a BSSID, and not a PAN ID, this step
	 * is redundant. Also, since the packet has
	 * arrived at this stage, it is guaranteed
	 * that it comes from the selected BSSID so
	 * we just copy the default PAN ID and send
	 * it up with the remaining of the packet.
	 */
	const uint16_t _pan_id = IEEE802154_PANID; 
	memcpy(uip_buf + _offset, &_pan_id, sizeof(uint16_t));
	_offset += sizeof(uint16_t);
	
	/* We can now copy the incoming packet data. */
	uip_len = packetbuf_datalen() + _offset;
	i = packetbuf_copyto(uip_buf + _offset);
	return i;
}
/*---------------------------------------------------------------------------*/
static int parse_80211(void)
{
	int len;
	uint16_t pan_id;
	rimeaddr_t* dst_mac_address;
	rimeaddr_t* src_mac_address;
	len = packetbuf_datalen();
	/* The first 8 bytes are the destination address 
	 * of the remote node [IEEE 802.11 MAC address]
	 * and it can well be an Ethernet broadcast one. 
	 */
	dst_mac_address = (rimeaddr_t*)(packetbuf_dataptr());
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, (rimeaddr_t *)dst_mac_address);
	/* We now need to remove these bytes from the 
	 * packet buffer so the plain IPv6 packet can
	 * be sent properly.
	 */
	if(packetbuf_hdrreduce(RIMEADDR_SIZE) == 0) {
		PRINTF("SLIP_FRAMER: ERROR; Could not remove MAC address info from incoming slip frame.\n");
		return -1;
	}
	/* The following 8 bytes contain the source address
	 * of the local [host] interface. 
	 */
	src_mac_address = (rimeaddr_t*)(packetbuf_dataptr());
	packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (rimeaddr_t *)src_mac_address);
	/* TODO Sanity check; this address should be the local node. */
	if (!ether_addr_equal((uint8_t*)src_mac_address, uip_lladdr.addr)) {
		PRINTF("SLIP_FRAMER; Packet coming from UART has a wrong source MAC Address!\n");
		PRINTADDR(src_mac_address);
		return -1;
	}
	/* We now need to remove these bytes from the 
	 * packet buffer so the plain IPv6 packet can
	 * be sent properly.
	 */
	if(packetbuf_hdrreduce(RIMEADDR_SIZE) == 0) {
		PRINTF("SLIP_FRAMER: ERROR; Could not remove MAC address info from incoming slip frame.\n");
		return -1;
	}
	
	/* The next 2 bytes contain the PAN ID. We should drop
	 * the packet if it does not carry the selected PAN ID
	 */
	pan_id = *((uint16_t*)(packetbuf_dataptr()));
	if (pan_id != mac_src_pan_id && pan_id != FRAME802154_BROADCASTPANDID) {
		/* Packet to another PAN */
		PRINTF("15.4: for another pan %04x\n", pan_id);
		return -1;
	}
	/* We now need to remove these bytes from the 
	 * packet buffer so the plain IPv6 packet can
	 * be sent properly.
	 */
	if(packetbuf_hdrreduce(sizeof(uint16_t)) == 0) {
		PRINTF("SLIP_FRAMER: ERROR; Could not remove PAN ID info from incoming slip frame.\n");
		return -1;
	}
	/* We would like to print some log information. */
	PRINTF("802.11-framer-IN: %02x%02x ",(uint8_t)pan_id,(uint8_t)(pan_id>>8));
	PRINTADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
	PRINTF(" After parse: %u (%u)\n", packetbuf_datalen(), len);
	/* We are now ready to send the packet down. */
	return 0;	
}
/*---------------------------------------------------------------------------*/
const struct framer slip_framer = {
  create_80211, parse_80211
};