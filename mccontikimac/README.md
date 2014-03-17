# MC-ContikiMAC

## About

* Short description: source code of MC-ContikiMAC, a Multi-Channel extension of ContikiMAC
* Contact: Beshr Al Nahas <beshr@sics.se>
* Author(s): Beshr Al Nahas, SICS
* Affiliation(s): SICS Swedish ICT AB
* License: BSD http://www.opensource.org/licenses/bsd-license.php
* Location of the code: in this repository
* Contiki version: Contiki 2.7
* Intended CALIPSO scenario: any with 802.15.4
* Intended platforms: any with 802.15.4
* Tested platforms: Tmote Sky
* Tested environment: Cooja+MSPSim, Indriya testbed

## Description

MC-ContikiMAC follows a channel-hopping approach integrated in low-power listening.
It aims at increasing robustness under external and internal interference.
Every node wakes up periodically and checks for activity on the medium.
Every wakeup happens on a different channel, based on a pseudo-random hopping sequence.
The pseudo-random seed is based on the receiver's MAC address, making it possible for the sender to predict the wakeup channel for a given known receiver at a given point in time.
Neighbors that are not yet discovered are reached through strobing over all used channels.

## Implementation

MC-ContikiMAC is implemented as a seperate mac protocol, that can be added to existing applications by changing the configuration of the NETSTACK_CONF_RDC to mccontikimac_driver and recompiling with the extra source files.

The implementation is available under (net/mac) and consists of the following files

* mccontikimac.c, .h: source code of MC-ContikiMAC.
* mccontikimac-hopseq.h: definition of the channel hopping sequences.
* mcphase.c, .h: functionality for phase-lock optimization with channel information.
* csma.c: slightly modified csma layer to make the back-off time proportional to number of channels.

### Configurable aspects

* MCMAC_NUM_CHANNELS: number of channels used. Default is 4.
* MCMAC_BC_NUM_CHANNELS: should we use different channels for broadcast, and how many. Default is 0, which means that broadcasts and unicasts are sent on the same channels.
* BC_RF_CHANNEL: if only one broadcast channel is configured, this defines the actual channel to be used. Default is  20.
* MCCONTIKIMAC_CONF_CYCLE_TIME: defines the length of one wake up period which is the inverse of the channel check rate.

## Usage example

A simple example of how to use MC-ContikiMAC is available under "rime".
The included Makefile and project-conf.h are good pointers to how to configure MC-ContikiMAC with your application.

