/*
 * Copyright (c) 2013, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Implementation of a multichannel ContikiMAC style power-saving radio duty cycling protocol
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Beshr Al Nahas <beshr@sics.se>
 *
 */

#include "contiki-conf.h"
#include "dev/leds.h"
#include "dev/radio.h"
#include "dev/watchdog.h"
#include "lib/random.h"
#include "net/mac/mccontikimac.h"
#include "net/mac/mccontikimac-hopseq.h"
#include "net/netstack.h"
#include "net/rime.h"
#include "sys/compower.h"
#include "sys/pt.h"
#include "sys/rtimer.h"

#include <string.h>
#include <stdio.h>

/* TODO ROOT_IP specifies the array representing the ip address of the always-on sink node for optional optimizations */
//#define ROOT_IP
/* ROOT_SLOT_TIME specifies how long does the non-duty-cycled node stay on one channel*/
#define ROOT_SLOT_TIME (RTIMER_SECOND / 100)

#define ACK_LEN (3)

/* XXX radio specific functions. These are not available in NETSTACK_RADIO struct*/
int
cc2420_set_channel(int channel);
int
cc2420_get_channel(void);

#define RADIO_set_channel(x) cc2420_set_channel(x)
#define RADIO_get_channel() cc2420_get_channel()

/* CYCLE_TIME for channel cca checks, in rtimer ticks. */
#ifdef MCCONTIKIMAC_CONF_CYCLE_TIME
#define CYCLE_TIME (MCCONTIKIMAC_CONF_CYCLE_TIME)
#else
#define CYCLE_TIME (RTIMER_ARCH_SECOND / NETSTACK_RDC_CHANNEL_CHECK_RATE)
#endif

#if UIP_CONF_IPV6
#define MY_ADDRESS_BYTE (rimeaddr_node_addr.u8[7] & 0xf)
#define OTHER_ADDRESS_BYTE (packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[7] & 0xf)
#define MY_INIT_CHANNEL_IDX ( MY_ADDRESS_BYTE  * HOP_SEQUENCE_COL_NUM )
#define SEND_INIT_CHANNEL_IDX \
		( OTHER_ADDRESS_BYTE * HOP_SEQUENCE_COL_NUM )
#else /* UIP_CONF_IPV6 */
#define MY_ADDRESS_BYTE (rimeaddr_node_addr.u8[0] & 0xf)
#define OTHER_ADDRESS_BYTE (packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0] & 0xf)
#define MY_INIT_CHANNEL_IDX ( MY_ADDRESS_BYTE  * HOP_SEQUENCE_COL_NUM )
#define SEND_INIT_CHANNEL_IDX \
		( OTHER_ADDRESS_BYTE * HOP_SEQUENCE_COL_NUM )
#endif /* UIP_CONF_IPV6 */

#ifndef EDC_TICKS_TO_METRIC
#define EDC_TICKS_TO_METRIC(edc) (uint16_t)((edc) / (CYCLE_TIME / 128))
#endif

/* Incoming data channel seed */
extern volatile uint8_t rec_channel_index;

#define ABS(x) (((x) > 0) ? (x) : -(x))

#define BUSYWAIT_UNTIL(cond, max_time)                                  \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time)));   \
  } while(0)

/* Use separate broadcast channel? */
#if MCMAC_BC_NUM_CHANNELS > 0
#define WITH_BROADCAST_CHANNEL      1
#else
#define WITH_BROADCAST_CHANNEL      0
#endif /* MCMAC_BC_NUM_CHANNELS > 0 */

/* TX/RX cycles are synchronized with neighbor wake periods */
#ifndef WITH_PHASE_OPTIMIZATION
#define WITH_PHASE_OPTIMIZATION      1
#endif
/* Two byte header added to allow recovery of padded short packets */
/* Wireshark will not understand such packets at present */
#ifdef MCCONTIKIMAC_CONF_WITH_MCCONTIKIMAC_HEADER
#define WITH_MCCONTIKIMAC_HEADER       MCCONTIKIMAC_CONF_WITH_MCCONTIKIMAC_HEADER
#else
#define WITH_MCCONTIKIMAC_HEADER       1
#endif
/* More aggressive radio sleeping when channel is busy with other traffic */
#ifndef WITH_FAST_SLEEP
#define WITH_FAST_SLEEP              1
#endif
/* Radio does CSMA and autobackoff */
#ifndef RDC_CONF_HARDWARE_CSMA
#define RDC_CONF_HARDWARE_CSMA       0
#endif
/* Radio returns TX_OK/TX_NOACK after autoack wait */
#ifndef RDC_CONF_HARDWARE_ACK
#define RDC_CONF_HARDWARE_ACK        0
#endif
/* MCU can sleep during radio off */
#ifndef RDC_CONF_MCU_SLEEP
#define RDC_CONF_MCU_SLEEP           0
#endif

#if NETSTACK_RDC_CHANNEL_CHECK_RATE >= 64
#undef WITH_PHASE_OPTIMIZATION
#define WITH_PHASE_OPTIMIZATION 0
#endif

#if WITH_MCCONTIKIMAC_HEADER
#define MCCONTIKIMAC_ID 0x01

struct hdr
{
  uint8_t id;
  uint8_t len;
};
#endif /* WITH_MCCONTIKIMAC_HEADER */

/* CHANNEL_CHECK_RATE is enforced to be a power of two.
 * If RTIMER_ARCH_SECOND is not also a power of two, there will be an inexact
 * number of channel checks per second due to the truncation of CYCLE_TIME.
 * This will degrade the effectiveness of mcphase optimization with neighbors that
 * do not have the same truncation error.
 * Define SYNC_CYCLE_STARTS to ensure an integral number of checks per second.
 */
#if RTIMER_ARCH_SECOND & (RTIMER_ARCH_SECOND - 1)
#define SYNC_CYCLE_STARTS                    1
#endif

/* BURST_RECV_TIME is the maximum time a receiver waits for the
 next packet of a burst when FRAME_PENDING is set. */
#define INTER_PACKET_DEADLINE               CLOCK_SECOND / 32

/* ContikiMAC performs periodic channel checks. Each channel check
 consists of two or more CCA checks. CCA_COUNT_MAX is the number of
 CCAs to be done for each periodic channel check. The default is
 two.*/
#define CCA_COUNT_MAX                      2

/* Before starting a transmission, Contikimac checks the availability
 of the channel with CCA_COUNT_MAX_TX consecutive CCAs */
#define CCA_COUNT_MAX_TX                   6

/* CCA_CHECK_TIME is the time it takes to perform a CCA check. */
/* Note this may be zero. AVRs have 7612 ticks/sec, but block until cca is done */
#define CCA_CHECK_TIME                     (RTIMER_ARCH_SECOND / 8192)

/* CCA_SLEEP_TIME is the time between two successive CCA checks. */
/* Add 1 when rtimer ticks are coarse */
#if RTIMER_ARCH_SECOND > 8000
#define CCA_SLEEP_TIME                     (RTIMER_ARCH_SECOND / 2000)
#else
#define CCA_SLEEP_TIME                     (RTIMER_ARCH_SECOND / 2000) + 1
#endif

/* CHECK_TIME is the total time it takes to perform CCA_COUNT_MAX
 CCAs. */
#define CHECK_TIME                         (CCA_COUNT_MAX * (CCA_CHECK_TIME + CCA_SLEEP_TIME))

/* CHECK_TIME_TX is the total time it takes to perform CCA_COUNT_MAX_TX
 CCAs. */
#define CHECK_TIME_TX                      (CCA_COUNT_MAX_TX * (CCA_CHECK_TIME + CCA_SLEEP_TIME))

/* LISTEN_TIME_AFTER_PACKET_DETECTED is the time that we keep checking
 for activity after a potential packet has been detected by a CCA
 check. */
#define LISTEN_TIME_AFTER_PACKET_DETECTED  (RTIMER_ARCH_SECOND / 80)

/* MAX_SILENCE_PERIODS is the maximum amount of periods (a period is
 CCA_CHECK_TIME + CCA_SLEEP_TIME) that we allow to be silent before
 we turn off the radio. */
#define MAX_SILENCE_PERIODS                5

/* MAX_NONACTIVITY_PERIODS is the maximum number of periods we allow
 the radio to be turned on without any packet being received, when
 WITH_FAST_SLEEP is enabled. */
#define MAX_NONACTIVITY_PERIODS            10

/* STROBE_TIME is the maximum amount of time a transmitted packet
 should be repeatedly transmitted as part of a transmission. */
#define STROBE_TIME                        (CYCLE_TIME + 2 * CHECK_TIME)

/* GUARD_TIME is the time before the expected mcphase of a neighbor that
 a transmitted should begin transmitting packets. */
#define GUARD_TIME                         (10 * CHECK_TIME + CHECK_TIME_TX)

/* INTER_PACKET_INTERVAL is the interval between two successive packet transmissions */
#define INTER_PACKET_INTERVAL              (RTIMER_ARCH_SECOND / 5000)

/* AFTER_ACK_DETECTECT_WAIT_TIME is the time to wait after a potential
 ACK packet has been detected until we can read it out from the
 radio. */
#define AFTER_ACK_DETECTECT_WAIT_TIME      (RTIMER_ARCH_SECOND / 1500)

/* MAX_PHASE_STROBE_TIME is the time that we transmit repeated packets
 to a neighbor for which we have a mcphase lock. */
#define MAX_PHASE_STROBE_TIME             (RTIMER_ARCH_SECOND / 60)
/* SHORTEST_PACKET_SIZE is the shortest packet that ContikiMAC
 allows. Packets have to be a certain size to be able to be detected
 by two consecutive CCA checks, and here is where we define this
 shortest size.
 Padded packets will have the wrong ipv6 checksum unless MCCONTIKIMAC_HEADER
 is used (on both sides) and the receiver will ignore them.
 With no header, reduce to transmit a proper multicast RPL DIS. */
#ifdef MCCONTIKIMAC_CONF_SHORTEST_PACKET_SIZE
#define SHORTEST_PACKET_SIZE  MCCONTIKIMAC_CONF_SHORTEST_PACKET_SIZE
#else
#define SHORTEST_PACKET_SIZE               43
#endif

static struct rtimer rt;
static struct pt pt;

/* Are we currently receiving a burst? */
volatile uint8_t we_are_receiving_burst = 0;
/* Has the receiver been awaken by a burst we're sending? */
volatile uint8_t is_receiver_awake = 0;

static volatile uint8_t mccontikimac_is_on = 0;
volatile uint8_t mccontikimac_keep_radio_on = 0;
volatile unsigned char we_are_sending = 0;
static volatile unsigned char radio_is_on = 0;

/** The index to the channel used for wakeup */
volatile uint8_t rec_channel_index = 0;

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTDEBUG(...) printf(__VA_ARGS__)
//#undef DISABLE_COOJA_DEBUG
//#include "cooja-debug.h"
//#define PRINTF(...) COOJA_DEBUG_PRINTF(__VA_ARGS__)
//#define PRINTDEBUG(...) COOJA_DEBUG_PRINTF(__VA_ARGS__)

#else
#define PRINTF(...)
#define PRINTDEBUG(...)
#endif

#if MCCONTIKIMAC_CONF_COMPOWER
static struct compower_activity current_packet;
#endif /* MCCONTIKIMAC_CONF_COMPOWER */

#if WITH_PHASE_OPTIMIZATION

#include "net/mac/mcphase.h"

#ifdef MCCONTIKIMAC_CONF_MAX_PHASE_NEIGHBORS
#define MAX_PHASE_NEIGHBORS MCCONTIKIMAC_CONF_MAX_PHASE_NEIGHBORS
#endif

#ifndef MAX_PHASE_NEIGHBORS
#define MAX_PHASE_NEIGHBORS 30
#endif

#endif /* WITH_PHASE_OPTIMIZATION */

#define DEFAULT_STREAM_TIME (4 * CYCLE_TIME)

#ifndef MIN
#define MIN(a, b) ((a) < (b)? (a) : (b))
#endif /* MIN */

struct seqno
{
  rimeaddr_t sender;
  uint8_t seqno;
};

#ifdef NETSTACK_CONF_MAC_SEQNO_HISTORY
#define MAX_SEQNOS_LL NETSTACK_CONF_MAC_SEQNO_HISTORY
#else /* NETSTACK_CONF_MAC_SEQNO_HISTORY */
#define MAX_SEQNOS_LL 16
#endif /* NETSTACK_CONF_MAC_SEQNO_HISTORY */
static struct seqno received_seqnos[MAX_SEQNOS_LL];

#if MCCONTIKIMAC_CONF_BROADCAST_RATE_LIMIT
static struct timer broadcast_rate_timer;
static int broadcast_rate_counter;
#endif /* MCCONTIKIMAC_CONF_BROADCAST_RATE_LIMIT */

static uint8_t rec_channel = RF_CHANNEL, send_channel = RF_CHANNEL,
    send_channel_index = 0, broadcast_channel_index = 0,
    hop_send_channel_index = 0;

static volatile uint8_t last_hop_time = 0;

const uint8_t hop_sequence[] = HOP_SEQUENCE_ARRAY;

#define HOP_SEQUENCE_SIZE 		  (HOP_SEQUENCE_ROW_NUM * HOP_SEQUENCE_COL_NUM)
#define REC_CHANNEL_LIMIT		    (rec_channel_index % HOP_SEQUENCE_COL_NUM)
#define CHANNEL_IDX(ch_ind) 	  ( (ch_ind) % HOP_SEQUENCE_SIZE )
#define ROW_CHANNEL_IDX(ch_ind) ( (ch_ind) % HOP_SEQUENCE_COL_NUM )

#if MCMAC_NUM_CHANNELS==1
#define MY_INIT_CHANNEL 		  RF_CHANNEL
#define SEND_CHANNEL(ch_ind) 	RF_CHANNEL
#define MY_CHANNEL 			      RF_CHANNEL
#define MY_PREVIOUS_CHANNEL 	RF_CHANNEL
#define HOP_CHANNEL 			    RF_CHANNEL
#define HOP_SEND_CHANNEL 		  RF_CHANNEL
#else /* MCMAC_NUM_CHANNELS!=1 */
#define MY_INIT_CHANNEL 		  ( hop_sequence[MY_INIT_CHANNEL_IDX] )
#define SEND_CHANNEL(ch_ind) 	( hop_sequence[CHANNEL_IDX(SEND_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(ch_ind))] )
#define MY_CHANNEL 			      ( hop_sequence[CHANNEL_IDX(MY_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(rec_channel_index))] )
#define MY_PREVIOUS_CHANNEL 	( hop_sequence[CHANNEL_IDX(MY_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(rec_channel_index-1))] )
#define HOP_CHANNEL 			    ( hop_sequence[CHANNEL_IDX(MY_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(++rec_channel_index))] )
#define HOP_SEND_CHANNEL 		  ( hop_sequence[CHANNEL_IDX(MY_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(++hop_send_channel_index))] )
#endif /* MCMAC_NUM_CHANNELS==1 */

/* CHANNEL_STROBES is the maximum amount of periods a transmitted packet
 should be repeatedly transmitted as part of a transmission when the channel is unknown
 or when sending broadcasts */
#define CHANNEL_STROBES 		(MCMAC_NUM_CHANNELS)

#if WITH_BROADCAST_CHANNEL
const uint8_t bc_hop_sequence[] = BC_HOP_SEQUENCE_ARRAY;
#define BC_HOP_SEQUENCE_SIZE        (BC_HOP_SEQUENCE_ROW_NUM * BC_HOP_SEQUENCE_COL_NUM)
#define BC_CHANNEL_IDX(ch_ind) 	    ((ch_ind) % BC_HOP_SEQUENCE_SIZE)
#define BC_ROW_CHANNEL_IDX(ch_ind)  ((MY_ADDRESS_BYTE * BC_HOP_SEQUENCE_COL_NUM + (ch_ind) % BC_HOP_SEQUENCE_COL_NUM)%BC_HOP_SEQUENCE_SIZE)
#define HOP_BC_CHANNEL 			        ( bc_hop_sequence[BC_ROW_CHANNEL_IDX(broadcast_channel_index++)] )
#define BC_CHANNEL 			            ( bc_hop_sequence[BC_ROW_CHANNEL_IDX(broadcast_channel_index)] )
#define BC_CHANNEL_STROBES 		      (MCMAC_BC_NUM_CHANNELS)
/* CCA checks broadcast channel defaults to the same count used for unicast channel */
#define BC_CCA_COUNT_MAX		        CCA_COUNT_MAX

#else
#define HOP_BC_CHANNEL			  ( hop_sequence[CHANNEL_IDX(MY_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(broadcast_channel_index++))] )
#define BC_CHANNEL 			      ( hop_sequence[CHANNEL_IDX(MY_INIT_CHANNEL_IDX + ROW_CHANNEL_IDX(broadcast_channel_index))] )
#define BC_CHANNEL_STROBES 		CHANNEL_STROBES
#define BC_CCA_COUNT_MAX		  0
#endif /* WITH_BROADCAST_CHANNEL */

/* CCA checks for both unicast and broadcast channel (if used) */
#define CCA_COUNT_MAX_TOT     (CCA_COUNT_MAX + BC_CCA_COUNT_MAX)

/* Recover the channel index of the receiving node (after sending a packet successfully)
 * Should be only called after receiving the ACK
 * and only if each channel appears only once in each hop-sequence **/
static uint8_t
receiver_channel_index(uint8_t channel)
{
  int i;
  for (i = 0; i < HOP_SEQUENCE_COL_NUM; i++) {
    if (SEND_CHANNEL(i) == channel) {
      return i + 1;
    }
  }
  return SEND_INIT_CHANNEL_IDX;
}
/*---------------------------------------------------------------------------*/
static void
set_channel(int channel)
{
  if (RADIO_get_channel() != channel) {
    RADIO_set_channel(channel);
  }
}
/*---------------------------------------------------------------------------*/
static void
on(void)
{
  if (mccontikimac_is_on && radio_is_on == 0) {
    radio_is_on = 1;
    NETSTACK_RADIO.on();
  }
}
/*---------------------------------------------------------------------------*/
static void
off(void)
{
  if (mccontikimac_is_on && radio_is_on != 0
      && mccontikimac_keep_radio_on == 0) {
    radio_is_on = 0;
    NETSTACK_RADIO.off();
  }
}
/*---------------------------------------------------------------------------*/
static volatile rtimer_clock_t cycle_start;
static char
powercycle(struct rtimer *t, void *ptr);
static void
schedule_powercycle(struct rtimer *t, rtimer_clock_t time)
{
  int r;

  if (mccontikimac_is_on || mccontikimac_keep_radio_on) {

    if (RTIMER_CLOCK_LT(RTIMER_TIME(t) + time, RTIMER_NOW() + 2)) {
      time = RTIMER_NOW() - RTIMER_TIME(t) + 2;
    }

    r = rtimer_set(t, RTIMER_TIME(t) + time, 1, (void
    (*)(struct rtimer *, void *)) powercycle, NULL);
    if (r != RTIMER_OK) {
      PRINTF("schedule_powercycle: could not set rtimer\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
schedule_powercycle_fixed(struct rtimer *t, rtimer_clock_t fixed_time)
{
  int r;

  if (mccontikimac_is_on || mccontikimac_keep_radio_on) {

    if (RTIMER_CLOCK_LT(fixed_time, RTIMER_NOW() + 1)) {
      fixed_time = RTIMER_NOW() + 1;
    }

    r = rtimer_set(t, fixed_time, 1, (void
    (*)(struct rtimer *, void *)) powercycle, NULL);
    if (r != RTIMER_OK) {
      PRINTF("mccontikimac: ! schedule_powercycle: could not set rtimer");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
powercycle_turn_radio_off(void)
{
#if MCCONTIKIMAC_CONF_COMPOWER
  uint8_t was_on = radio_is_on;
#endif /* MCCONTIKIMAC_CONF_COMPOWER */

  if (we_are_sending == 0 && we_are_receiving_burst == 0) {
    off();

#if MCCONTIKIMAC_CONF_COMPOWER
    if (was_on && !radio_is_on)
    {
      compower_accumulate(&compower_idle_activity);
    }
#endif /* MCCONTIKIMAC_CONF_COMPOWER */
  }
}
/*---------------------------------------------------------------------------*/
/* Set the channel and turn on the radio to listen */
static void
powercycle_turn_radio_on(void)
{
  if (we_are_sending == 0 && we_are_receiving_burst == 0) {
    set_channel(rec_channel);
    on();
  }
}
/*---------------------------------------------------------------------------*/
/* powercycle function determines the listening schema.
 * Supports both RDC and always-on listening
 * The channel is picked from a pseudo-random sequence each wakeup cycle
 * NO fixed broadcast channel
 */
static char
powercycle(struct rtimer *t, void *ptr)
{
#if SYNC_CYCLE_STARTS
  static volatile rtimer_clock_t sync_cycle_start;
  static volatile uint8_t sync_cycle_mcphase;
#endif

  PT_BEGIN(&pt);
#if SYNC_CYCLE_STARTS
  sync_cycle_start = RTIMER_NOW();
#else
  cycle_start = RTIMER_NOW();
#endif

  /* Initialize receive channel */
  rec_channel = HOP_CHANNEL;

#if WITH_BROADCAST_CHANNEL
  static uint8_t bc_check;
  bc_check = 0;
  /* which channel to use for BC*/
  static uint8_t bc_channel;
  bc_channel = HOP_BC_CHANNEL;
#endif /* WITH_BROADCAST_CHANNEL */

  while (1) {
    static uint8_t packet_seen;
    /* which CCA check it is*/
    static uint16_t cca_count;

    packet_seen = 0;

#if SYNC_CYCLE_STARTS
    /* Compute cycle start when RTIMER_ARCH_SECOND is not a multiple of CHANNEL_CHECK_RATE */
    if (sync_cycle_mcphase++ == NETSTACK_RDC_CHANNEL_CHECK_RATE)
    {
      sync_cycle_mcphase = 0;
      sync_cycle_start += RTIMER_ARCH_SECOND;
      cycle_start = sync_cycle_start;
    }
    else
    {
#if (RTIMER_ARCH_SECOND * NETSTACK_RDC_CHANNEL_CHECK_RATE) > 65535
      cycle_start = sync_cycle_start + ((unsigned long)(sync_cycle_mcphase*RTIMER_ARCH_SECOND))/NETSTACK_RDC_CHANNEL_CHECK_RATE;
#else
      cycle_start = sync_cycle_start + (sync_cycle_mcphase*RTIMER_ARCH_SECOND)/NETSTACK_RDC_CHANNEL_CHECK_RATE;
#endif
    }
#else
    cycle_start += CYCLE_TIME;
#endif

    if (!mccontikimac_keep_radio_on) {

#if WITH_BROADCAST_CHANNEL
      bc_channel = HOP_BC_CHANNEL;
#endif /* WITH_BROADCAST_CHANNEL */
      rec_channel = HOP_CHANNEL;
      last_hop_time = RTIMER_NOW();
      for (cca_count = 0; cca_count < CCA_COUNT_MAX_TOT; ++cca_count) {
        if (we_are_sending == 0 && we_are_receiving_burst == 0) {
#if WITH_BROADCAST_CHANNEL
          if(cca_count >= CCA_COUNT_MAX)
          {
            rec_channel = bc_channel;
          }
#endif /* WITH_BROADCAST_CHANNEL */

          /* Set channel and turn radio on */
          powercycle_turn_radio_on();
          /* Check if a packet is seen in the air. If so, we keep the
           radio on for a while (LISTEN_TIME_AFTER_PACKET_DETECTED) to
           be able to receive the packet. We also continuously check
           the radio medium to make sure that we weren't waken up by a
           false positive: a spurious radio interference that was not
           caused by an incoming packet. */
          if (NETSTACK_RADIO.channel_clear() == 0) {
            packet_seen = 1;

            break;
          }
          powercycle_turn_radio_off();
        }
        schedule_powercycle_fixed(t, RTIMER_NOW() + CCA_SLEEP_TIME);
        PT_YIELD(&pt);
      }

      if (packet_seen) {
        static rtimer_clock_t start;
        static uint8_t silence_periods, periods, radio_received_packet;
        start = RTIMER_NOW();
        radio_received_packet = 0;
        periods = silence_periods = 0;
        while (we_are_sending == 0 && radio_is_on
            && RTIMER_CLOCK_LT(RTIMER_NOW(),
                (start + LISTEN_TIME_AFTER_PACKET_DETECTED))) {

          /* Check for a number of consecutive periods of
           non-activity. If we see two such periods, we turn the
           radio off. Also, if a packet has been successfully
           received (as indicated by the
           NETSTACK_RADIO.pending_packet() function), we stop
           snooping. */

#if !RDC_CONF_HARDWARE_CSMA
          if (!radio_received_packet && !NETSTACK_RADIO.pending_packet()) {
            /* A cca cycle will disrupt rx on some radios, e.g. mc1322x, rf230 */
            /* TODO: Modify those drivers to just return the internal RSSI when already in rx mode */
            if (NETSTACK_RADIO.channel_clear()) {
              ++silence_periods;
            } else {
              silence_periods = 0;
            }
          }
#endif

          ++periods;

          if (NETSTACK_RADIO.receiving_packet()) {
            silence_periods = 0;
          }
          if (silence_periods > MAX_SILENCE_PERIODS) {
            powercycle_turn_radio_off();
            break;
          }
          if (NETSTACK_RADIO.pending_packet()) {
            break;
          }
          if (WITH_FAST_SLEEP && periods > MAX_NONACTIVITY_PERIODS
              && !(NETSTACK_RADIO.receiving_packet()
                  || NETSTACK_RADIO.pending_packet())) {
            powercycle_turn_radio_off();
            break;
          }

          schedule_powercycle_fixed(t,
              RTIMER_NOW() + CCA_CHECK_TIME + CCA_SLEEP_TIME);
          PT_YIELD(&pt);
        }
        if (radio_is_on) {
          if (!(NETSTACK_RADIO.receiving_packet()
              || NETSTACK_RADIO.pending_packet())
              || !RTIMER_CLOCK_LT(RTIMER_NOW(),
                  (start + LISTEN_TIME_AFTER_PACKET_DETECTED))) {
            powercycle_turn_radio_off();
          }
        }
      }

      if (RTIMER_CLOCK_LT(RTIMER_NOW() - cycle_start,
      CYCLE_TIME - CHECK_TIME * 4)) {
        /* Schedule the next powercycle interrupt, or sleep the mcu until then.
         Sleeping will not exit from this interrupt, so ensure an occasional wake cycle
         or foreground processing will be blocked until a packet is detected */
#if RDC_CONF_MCU_SLEEP
        static uint8_t sleepcycle;
        if ((sleepcycle++<16) && !we_are_sending && !radio_is_on)
        {
          rtimer_arch_sleep(CYCLE_TIME - (RTIMER_NOW() - cycle_start));
        }
        else
        {
          sleepcycle = 0;
          schedule_powercycle_fixed(t, CYCLE_TIME + cycle_start);
          PT_YIELD(&pt);
        }
#else
        schedule_powercycle_fixed(t, CYCLE_TIME + cycle_start);
        PT_YIELD(&pt);
#endif
      }
    } else {

      static int i;
      for (i = 0; i < MCMAC_NUM_CHANNELS + MCMAC_BC_NUM_CHANNELS; i++) {
        if (i < MCMAC_NUM_CHANNELS) {
          rec_channel = HOP_CHANNEL;
        } else {
          rec_channel = HOP_BC_CHANNEL;
        }
        if (we_are_sending == 0 && we_are_receiving_burst == 0) {
          if (!NETSTACK_RADIO.receiving_packet()
              && !NETSTACK_RADIO.pending_packet()) {
            set_channel(rec_channel);
          }
        }
        if (RTIMER_CLOCK_LT(RTIMER_NOW() - cycle_start,
        CYCLE_TIME - ROOT_SLOT_TIME)) {
          schedule_powercycle_fixed(t, CYCLE_TIME + cycle_start);
        } else {
          schedule_powercycle(t, ROOT_SLOT_TIME);
        }
        PT_YIELD(&pt);
      }
    }
  }
  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
static int
broadcast_rate_drop(void)
{
#if MCCONTIKIMAC_CONF_BROADCAST_RATE_LIMIT
  if(!timer_expired(&broadcast_rate_timer))
  {
    broadcast_rate_counter++;
    if(broadcast_rate_counter < MCCONTIKIMAC_CONF_BROADCAST_RATE_LIMIT)
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
  else
  {
    timer_set(&broadcast_rate_timer, CLOCK_SECOND);
    broadcast_rate_counter = 0;
    return 0;
  }
#else /* MCCONTIKIMAC_CONF_BROADCAST_RATE_LIMIT */
  return 0;
#endif /* MCCONTIKIMAC_CONF_BROADCAST_RATE_LIMIT */
}
/*---------------------------------------------------------------------------*/
static int
send_packet(mac_callback_t mac_callback, void *mac_callback_ptr,
    struct rdc_buf_list *buf_list)
{
  rtimer_clock_t t0, t1;
  rtimer_clock_t encounter_time = 0; //time of the successful strobe
  uint8_t encounter_channelseed = 0;
  int strobes;
  uint8_t got_strobe_ack = 0;
  int hdrlen, len;
  uint8_t is_broadcast = 0;
  //uint8_t is_reliable = 0;
  uint8_t is_known_receiver = 0;
  uint8_t collisions = 0;
  int transmit_len;
  int ret;
  uint8_t mccontikimac_was_on = mccontikimac_is_on;
  uint8_t seqno;
  uint8_t send_trial = 0;
  uint8_t max_send_trials;

#if WITH_MCCONTIKIMAC_HEADER
  struct hdr *chdr;
#endif /* WITH_MCCONTIKIMAC_HEADER */

  /* Exit if RDC and radio were explicitly turned off */
  if (!mccontikimac_is_on && !mccontikimac_keep_radio_on) {
    PRINTF("mccontikimac: ! radio is turned off\n");
    return MAC_TX_ERR_FATAL;
  }

  if (packetbuf_totlen() == 0) {
    PRINTF("mccontikimac: ! send_packet data len 0\n");
    return MAC_TX_ERR_FATAL;
  }

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &rimeaddr_node_addr);

  /* Check if sending broadcast --> no ACK */
  if (rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_null)) {
    is_broadcast = 1;
    PRINTDEBUG("mccontikimac: send broadcast\n");

    if (broadcast_rate_drop()) {
      return MAC_TX_COLLISION;
    }
  } else {
#if UIP_CONF_IPV6
    PRINTDEBUG("mccontikimac: send unicast to %02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[1],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[2],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[3],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[4],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[5],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[6],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[7]);
#else /* UIP_CONF_IPV6 */
    PRINTDEBUG("mccontikimac: send unicast to %u.%u\n",
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0],
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[1]);
#endif /* UIP_CONF_IPV6 */
  }
//	is_reliable = packetbuf_attr(PACKETBUF_ATTR_RELIABLE)
//			|| packetbuf_attr(PACKETBUF_ATTR_ERELIABLE);

  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);

#if WITH_MCCONTIKIMAC_HEADER
  hdrlen = packetbuf_totlen();
  if (packetbuf_hdralloc(sizeof(struct hdr)) == 0) {
    /* Failed to allocate space for mccontikimac header */
    PRINTF("mccontikimac: send failed, too large header\n");
    return MAC_TX_ERR_FATAL;
  }
  chdr = packetbuf_hdrptr();
  chdr->id = MCCONTIKIMAC_ID;
  chdr->len = hdrlen;

  /* Create the MAC header for the data packet. */
  hdrlen = NETSTACK_FRAMER.create();
  if (hdrlen < 0) {
    /* Failed to send */
    PRINTF("mccontikimac: send failed, too large header\n");
    packetbuf_hdr_remove(sizeof(struct hdr));
    return MAC_TX_ERR_FATAL;
  }
  hdrlen += sizeof(struct hdr);
#else
  /* Create the MAC header for the data packet. */
  hdrlen = NETSTACK_FRAMER.create();
  if(hdrlen < 0)
  {
    /* Failed to send */
    PRINTF("mccontikimac: send failed, too large header\n");
    return MAC_TX_ERR_FATAL;
  }
#endif

  /* Make sure that the packet is longer or equal to the shortest
   packet length. */
  transmit_len = packetbuf_totlen();
  if (transmit_len < SHORTEST_PACKET_SIZE) {
    /* Pad with zeroes */
    uint8_t *ptr;
    ptr = packetbuf_dataptr();
    memset(ptr + packetbuf_datalen(), 0,
    SHORTEST_PACKET_SIZE - packetbuf_totlen());

    PRINTF("mccontikimac: shorter than shortest (%d)\n", packetbuf_totlen());
    transmit_len = SHORTEST_PACKET_SIZE;
  }

  packetbuf_compact();

  NETSTACK_RADIO.prepare(packetbuf_hdrptr(), transmit_len);

  /* Remove the MAC-layer header since it will be recreated next time around. */
  packetbuf_hdr_remove(hdrlen);

  /* If receiver is awake then use the last send channel, no need to update */

  /* If it is broadcast then hop the channel
   * and keep the same broadcast channel for the whole time of broadcast */
  if (is_broadcast) {
#if WITH_BROADCAST_CHANNEL
    /* No need to hop BC channel here since wake-up hops it periodically */
    send_channel = BC_CHANNEL;
#else
    /* Hop BC channel here since it makes more sense to change BC Channel
     * when we need to use it*/
    send_channel = HOP_BC_CHANNEL;
#endif	/* WITH_BROADCAST_CHANNEL */
    max_send_trials = BC_CHANNEL_STROBES;
  } else {
    max_send_trials = CHANNEL_STROBES;
  }

  /* keep track of the time */
  t0 = RTIMER_NOW();

  /* FIXME a hack to disable phase lock with the sink which is always on */
#ifdef ROOT_IP
  if(rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &ROOT_IP))
  {
    is_receiver_awake = 1;
    send_channel = MY_CHANNEL;
  }
#endif

  if (!send_trial && !is_broadcast && !is_receiver_awake) {
#if WITH_PHASE_OPTIMIZATION
    /* Retrieve channel index, and expected wakeup time */
    ret = mcphase_wait(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), CYCLE_TIME,
    GUARD_TIME, mac_callback, mac_callback_ptr, buf_list, &send_channel_index);
    if (ret == PHASE_DEFERRED) {
      return MAC_TX_DEFERRED;
    }
    if (ret != PHASE_UNKNOWN) {
      is_known_receiver = 1;
      send_channel = SEND_CHANNEL(send_channel_index);
    } else {
      /* When receiver's channel is unknown,
       * pick any channel (e.g. last receive channel)
       * and stick to it until finding the receiver.
       * However, it should keep the same channel along all strobes. */
      send_channel = HOP_SEND_CHANNEL;
    }
#endif /* WITH_PHASE_OPTIMIZATION */
  }

#if MCMAC_NUM_CHANNELS==1
  send_channel = RF_CHANNEL;
#endif /* MCMAC_NUM_CHANNELS==1 */

  /* By setting we_are_sending to one, we ensure that the rtimer
   powercycle interrupt do not interfere with us sending the packet. */
  we_are_sending = 1;

  /* If we have a pending packet in the radio, we should not send now,
   because we will trash the received packet. Instead, we signal
   that we have a collision, which lets the packet be received. This
   packet will be retransmitted later by the MAC protocol
   instead. */
  if (NETSTACK_RADIO.receiving_packet() || NETSTACK_RADIO.pending_packet()) {
    we_are_sending = 0;
    PRINTF("mccontikimac: collision receiving %d, pending %d\n",
        NETSTACK_RADIO.receiving_packet(), NETSTACK_RADIO.pending_packet());
    return MAC_TX_COLLISION;
  }

  /* Switch off the radio to ensure that we didn't start sending while
   the radio was doing a channel check. */
  off();

  strobes = 0;

  /* Send a train of strobes until the receiver answers with an ACK. */
  collisions = 0;

  got_strobe_ack = 0;

  /* Set mccontikimac_is_on to one to allow the on() and off() functions
   to control the radio. We restore the old value of
   mccontikimac_is_on when we are done. */
  mccontikimac_was_on = mccontikimac_is_on;
  mccontikimac_is_on = 1;

  /* Change radio channel */
  set_channel(send_channel);

#if !RDC_CONF_HARDWARE_CSMA
  /* Check if there are any transmissions by others. */
  /* TODO: why does this give collisions before sending with the mc1322x? */
  if (!is_receiver_awake) {
    int i;
    for (i = 0; i < CCA_COUNT_MAX_TX; ++i) {
      t1 = RTIMER_NOW();
      on();
#if CCA_CHECK_TIME > 0
      while(RTIMER_CLOCK_LT(RTIMER_NOW(), t1 + CCA_CHECK_TIME))
      {}
#endif
      if (NETSTACK_RADIO.channel_clear() == 0) {
        collisions++;
        off();
        break;
      }
      off();
      t1 = RTIMER_NOW();
      while (RTIMER_CLOCK_LT(RTIMER_NOW(), t1 + CCA_SLEEP_TIME)) {
      }
    }
  }

  if (collisions > 0) {
    we_are_sending = 0;
    off();
    PRINTF("mccontikimac: collisions before sending\n");
    mccontikimac_is_on = mccontikimac_was_on;
    return MAC_TX_COLLISION;
  }
#endif /* RDC_CONF_HARDWARE_CSMA */

#if !RDC_CONF_HARDWARE_ACK
  //if(!is_broadcast) {
  /* Turn radio on to receive expected unicast ack.
   Not necessary with hardware ack detection, and may trigger an unnecessary cca or rx cycle */
  on();
  //}
#endif

  uint8_t ackbuf[ACK_LEN] = { 0 };

  seqno = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  t1 = RTIMER_NOW();
  for (strobes = 0, collisions = 0;
      got_strobe_ack == 0 && collisions == 0
          && RTIMER_CLOCK_LT(RTIMER_NOW(), t1 + max_send_trials * STROBE_TIME);
      strobes++) {

    watchdog_periodic();

    /* FIXME a hack that defines a special strobe time for the sink which is always on */
    uint32_t max_strobe_time;
#ifdef ROOT_IP
    if(rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &ROOT_IP))
    {
      max_strobe_time = (MCMAC_NUM_CHANNELS + MCMAC_BC_NUM_CHANNELS) * ROOT_SLOT_TIME;
    }
    else
    {
      max_strobe_time = MAX_PHASE_STROBE_TIME;
    }
#else
    max_strobe_time = MAX_PHASE_STROBE_TIME;
#endif /* ifdef ROOT_ID */

    if ((is_receiver_awake || is_known_receiver)
        && !RTIMER_CLOCK_LT(RTIMER_NOW(), t1 + max_strobe_time)) {
      off();
      PRINTF("mccontikimac: miss to %d\n",
          packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0]);
      break;
    }

    len = 0;

    {
      rtimer_clock_t wt, txtime, previous_txtime;
      int ret;

      previous_txtime = RTIMER_NOW();
      ret = NETSTACK_RADIO.transmit(transmit_len);

#if RDC_CONF_HARDWARE_ACK
      /* For radios that block in the transmit routine and detect the ACK in hardware */
      if(ret == RADIO_TX_OK)
      {
        if(!is_broadcast)
        {
          got_strobe_ack = 1;
          encounter_time = txtime;
          /* Get channel seed */
          encounter_channelseed = receiver_channel_index(send_channel);
          break;
        }
      }
      else if (ret == RADIO_TX_NOACK)
      {
      }
      else if (ret == RADIO_TX_COLLISION)
      {
        PRINTF("mccontikimac: collisions while sending\n");
        collisions++;
      }
      wt = RTIMER_NOW();
      while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + INTER_PACKET_INTERVAL))
      {}
#else
      /* Wait for the ACK packet */
      int c1 = 0, c2 = 0, c3 = 0;
      wt = RTIMER_NOW();
      NETSTACK_RADIO.on();
      while (RTIMER_CLOCK_LT(RTIMER_NOW(), wt + INTER_PACKET_INTERVAL)) {
      }

      if (!is_broadcast) {
        c1 = NETSTACK_RADIO.receiving_packet();
        c2 = NETSTACK_RADIO.pending_packet();
        c3 = NETSTACK_RADIO.channel_clear() == 0;
        if (c1 || c2 || c3) {
          while (RTIMER_CLOCK_LT(RTIMER_NOW(),
              wt + AFTER_ACK_DETECTECT_WAIT_TIME)) {
          }
          len = NETSTACK_RADIO.read(ackbuf, ACK_LEN);
          if (len == ACK_LEN && seqno == ackbuf[2]) {
            got_strobe_ack++;
            /* Get channel seed */
            encounter_channelseed = receiver_channel_index(send_channel);
            encounter_time = previous_txtime;
            break;
          }
        }
      }
#endif /* RDC_CONF_HARDWARE_ACK */
      previous_txtime = txtime;
    }
  }

  uint16_t strobe_duration = EDC_TICKS_TO_METRIC(RTIMER_NOW() - t0);

  off();

  PRINTF("mccontikimac: send (strobes=%u, len=%u, %s, %s), done\n", strobes,
      packetbuf_totlen(), got_strobe_ack ? "ack" : "no ack",
      collisions ? "collision" : "no collision");

#if MCCONTIKIMAC_CONF_COMPOWER
  /* Accumulate the power consumption for the packet transmission. */
  compower_accumulate(&current_packet);

  /* Convert the accumulated power consumption for the transmitted
   packet to packet attributes so that the higher levels can keep
   track of the amount of energy spent on transmitting the
   packet. */
  compower_attrconv(&current_packet);

  /* Clear the accumulated power consumption so that it is ready for
   the next packet. */
  compower_clear(&current_packet);
#endif /* MCCONTIKIMAC_CONF_COMPOWER */

  mccontikimac_is_on = mccontikimac_was_on;
  we_are_sending = 0;

  /* Determine the return value that we will return from the
   function. We must pass this value to the mcphase module before we
   return from the function.  */
  if (collisions > 0) {
    ret = MAC_TX_COLLISION;
  } else if (!is_broadcast && !got_strobe_ack) {
    ret = MAC_TX_NOACK;
  } else {
    ret = MAC_TX_OK;
  }

#if WITH_PHASE_OPTIMIZATION

  if (is_known_receiver && got_strobe_ack) {
    PRINTF("no miss %d wake-ups %d\n",
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0], strobes);
  }

  if (!is_broadcast) {
    if (collisions == 0 && is_receiver_awake == 0) {
      mcphase_update(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), encounter_time,
          ret, encounter_channelseed);
    }
  }
#endif /* WITH_PHASE_OPTIMIZATION */

  if (got_strobe_ack) {
    PRINTF("mccontikimac: acked by %u s %u c %d ch %d pl %d",
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0], strobe_duration,
        collisions, send_channel, is_known_receiver);
  } else {
    if (!is_broadcast) {
      PRINTF("mccontikimac:! noack s %u c %d ch %d pl %d", strobe_duration,
          collisions, send_channel, is_known_receiver);
    } else {
      PRINTF("mccontikimac: sending broadcast s %u c %d ch %d\n",
          strobe_duration, collisions, send_channel);
    }
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static void
qsend_packet(mac_callback_t sent, void *ptr)
{
  int ret = send_packet(sent, ptr, NULL);
  if (ret != MAC_TX_DEFERRED) {
    mac_call_sent_callback(sent, ptr, ret, 1);
  }
}
/*---------------------------------------------------------------------------*/
static void
qsend_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  struct rdc_buf_list *curr = buf_list;
  struct rdc_buf_list *next;
  int ret;
  if (curr == NULL) {
    return;
  }
  /* Do not send during reception of a burst */
  if (we_are_receiving_burst) {
    /* Prepare the packetbuf for callback */
    queuebuf_to_packetbuf(curr->buf);
    /* Return COLLISION so the MAC may try again later */
    mac_call_sent_callback(sent, ptr, MAC_TX_COLLISION, 1);
    return;
  }
  /* The receiver needs to be awaken before we send */
  is_receiver_awake = 0;
  do { /* A loop sending a burst of packets from buf_list */
    next = list_item_next(curr);

    /* Prepare the packetbuf */
    queuebuf_to_packetbuf(curr->buf);
    if (next != NULL) {
      packetbuf_set_attr(PACKETBUF_ATTR_PENDING, 1);
    }

    /* Send the current packet */
    ret = send_packet(sent, ptr, curr);
    if (ret != MAC_TX_DEFERRED) {
      mac_call_sent_callback(sent, ptr, ret, 1);
    }

    if (ret == MAC_TX_OK) {
      if (next != NULL) {
        /* We're in a burst, no need to wake the receiver up again */
        is_receiver_awake = 1;
        curr = next;
      }
    } else {
      /* The transmission failed, we stop the burst */
      next = NULL;
    }
  }
  while (next != NULL);
  is_receiver_awake = 0;
}
/*---------------------------------------------------------------------------*/
/* Timer callback triggered when receiving a burst, after having waited for a next
 packet for a too long time. Turns the radio off and leaves burst reception mode */
static void
recv_burst_off(void *ptr)
{
  off();
  we_are_receiving_burst = 0;
}
/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
  static struct ctimer ct;
  if (!we_are_receiving_burst) {
    off();
  }

  /*  printf("cycle_start 0x%02x 0x%02x\n", cycle_start, cycle_start % CYCLE_TIME);*/

#ifdef NETSTACK_DECRYPT
  NETSTACK_DECRYPT();
#endif /* NETSTACK_DECRYPT */

  if (packetbuf_totlen() > 0 && NETSTACK_FRAMER.parse() >= 0) {

#if WITH_MCCONTIKIMAC_HEADER
    struct hdr *chdr;
    chdr = packetbuf_dataptr();
    if (chdr->id != MCCONTIKIMAC_ID) {
      PRINTF("mccontikimac: failed to parse hdr (%u)\n", packetbuf_totlen());
      return;
    }
    packetbuf_hdrreduce(sizeof(struct hdr));
    packetbuf_set_datalen(chdr->len);
#endif /* WITH_MCCONTIKIMAC_HEADER */

    if (packetbuf_datalen() > 0 && packetbuf_totlen() > 0
        && (rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
            &rimeaddr_node_addr)
            || rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                &rimeaddr_null))) {
      /* This is a regular packet that is destined to us or to the
       broadcast address. */

      /* If FRAME_PENDING is set, we are receiving a packets in a burst */
      we_are_receiving_burst = packetbuf_attr(PACKETBUF_ATTR_PENDING);
      if (we_are_receiving_burst) {
        on();
        /* Set a timer to turn the radio off in case we do not receive
         a next packet */
        ctimer_set(&ct, INTER_PACKET_DEADLINE, recv_burst_off, NULL);

      } else {
        off();
        ctimer_stop(&ct);

      }

      /* Check for duplicate packet by comparing the sequence number
       of the incoming packet with the last few ones we saw. */
      {
        int i;
        for (i = 0; i < MAX_SEQNOS_LL; ++i) {
          if (packetbuf_attr(PACKETBUF_ATTR_PACKET_ID)
              == received_seqnos[i].seqno
              && rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER),
                  &received_seqnos[i].sender)) {
            /* Drop the packet. */
            /*        printf("Drop duplicate ContikiMAC layer packet\n");*/
            return;
          }
        }
        for (i = MAX_SEQNOS_LL - 1; i > 0; --i) {
          memcpy(&received_seqnos[i], &received_seqnos[i - 1],
              sizeof(struct seqno));
        }
        received_seqnos[0].seqno = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
        rimeaddr_copy(&received_seqnos[0].sender,
            packetbuf_addr(PACKETBUF_ADDR_SENDER));
      }

#if MCCONTIKIMAC_CONF_COMPOWER
      /* Accumulate the power consumption for the packet reception. */
      compower_accumulate(&current_packet);
      /* Convert the accumulated power consumption for the received
       packet to packet attributes so that the higher levels can
       keep track of the amount of energy spent on receiving the
       packet. */
      compower_attrconv(&current_packet);

      /* Clear the accumulated power consumption so that it is ready
       for the next packet. */
      compower_clear(&current_packet);
#endif /* MCCONTIKIMAC_CONF_COMPOWER */

      PRINTDEBUG("mccontikimac: data (%u)\n", packetbuf_datalen());
      NETSTACK_MAC.input();
      return;
    } else {
      PRINTDEBUG("mccontikimac: data not for us\n");
    }
  } else {
    PRINTF("mccontikimac: failed to parse (%u)\n", packetbuf_totlen());
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  radio_is_on = 0;
  PT_INIT(&pt);

  rtimer_set(&rt, RTIMER_NOW() + CYCLE_TIME, 1, (void
  (*)(struct rtimer *, void *)) powercycle, NULL);

  mccontikimac_is_on = 1;

#if WITH_PHASE_OPTIMIZATION
  mcphase_init();
#endif /* WITH_PHASE_OPTIMIZATION */
  printf("McContikiMAC: parameters: channels %d, bc %d\n", MCMAC_NUM_CHANNELS,
  MCMAC_BC_NUM_CHANNELS);
}
/*---------------------------------------------------------------------------*/
static int
turn_on(void)
{
  if (mccontikimac_is_on == 0) {
    mccontikimac_is_on = 1;
    mccontikimac_keep_radio_on = 0;
    rtimer_set(&rt, RTIMER_NOW() + CYCLE_TIME, 1, (void
    (*)(struct rtimer *, void *)) powercycle, NULL);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
turn_off(int keep_radio_on)
{
  mccontikimac_is_on = 0;
  mccontikimac_keep_radio_on = keep_radio_on;
  if (keep_radio_on) {
    radio_is_on = 1;
    /* schedule power cycle to change channels,
     * because the radio is configured to be never off */
    schedule_powercycle_fixed(&rt, RTIMER_NOW() + CYCLE_TIME);
    return NETSTACK_RADIO.on();
  } else {
    radio_is_on = 0;
    return NETSTACK_RADIO.off();
  }
}
/*---------------------------------------------------------------------------*/
static unsigned short
duty_cycle(void)
{
  return (1ul * CLOCK_SECOND * CYCLE_TIME) / RTIMER_ARCH_SECOND;
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver mccontikimac_driver = { "McContikiMAC", init,
    qsend_packet, qsend_list, input_packet, turn_on, turn_off, duty_cycle, };
/*---------------------------------------------------------------------------*/
uint16_t
mccontikimac_debug_print(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
