/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *
 * $Id: project-conf.h,v 1.1 2010/10/21 18:23:44 joxe Exp $
 */

#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#undef RF_CHANNEL
#define RF_CHANNEL CMD_RF_CHANNEL
#define MCMAC_NUM_CHANNELS CMD_NUM_CHANNELS
#define SEND_INTERVAL (CMD_IPI * CLOCK_SECOND)

#define MCMAC_BC_NUM_CHANNELS  CMD_BC_CHANNELS
/* useful only if one BC channel is used */
#define BC_RF_CHANNEL CMD_BC_RF_CHANNEL

#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID     CMD_PAN_ID

#define SOFTACK 0

#undef RF_POWER
#define RF_POWER 31

#define EXTRA_ACK_LEN 0

//#if IN_COOJA
//#define DISABLE_COOJA_DEBUG 0
//#else
//#define DISABLE_COOJA_DEBUG 1
//#endif
//#include "cooja-debug.h"

#undef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 0

#undef DCOSYNCH_CONF_ENABLED
#define DCOSYNCH_CONF_ENABLED 0

/* 32-bit rtimer */
#define RTIMER_CONF_SECOND (4096UL*8)
typedef uint32_t rtimer_clock_t;
#define RTIMER_CLOCK_LT(a,b)     ((int32_t)(((rtimer_clock_t)a)-((rtimer_clock_t)b)) < 0)

#define CONTIKIMAC_CONF_CYCLE_TIME (CMD_CYCLE_TIME * RTIMER_ARCH_SECOND / 1000)

#undef CSMA_CONF_MAX_NEIGHBOR_QUEUES
#define CSMA_CONF_MAX_NEIGHBOR_QUEUES 2

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 4

#define PHASE_CONF_DRIFT_CORRECT 0

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC    mccontikimac_driver

//CSMA retransmissions
#undef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS 5

#undef CC2420_CONF_SFD_TIMESTAMPS
#define CC2420_CONF_SFD_TIMESTAMPS 1

#endif /* __PROJECT_CONF_H__ */
