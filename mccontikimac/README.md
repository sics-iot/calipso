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

MC-ContikiMAC follows a channel-hoping approach integrated in low-power listening.
It aims at increasing robustness under external and internal interference.
Every node wakes up periodically and checks for activity on the medium.
Every wakeup happens on a different channel, based on a pseudo-random hoping sequence.
The pseudo-random seed is based on the receiver's MAC address, making it possible for the sender to predict the wakeup channel for a given known receiver at a given point in time.
Neighbors that are not yet discovered are reached through strobing over all used channels.
