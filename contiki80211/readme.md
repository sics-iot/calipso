# Multi-Hop Power Save Mode (MH-PSM) for 802.11

## About

* Short description: We extend the IEEE 802.11 Power Saving Mode (PSM) with a traffic announcement scheme that facilitates multi-hop communication. The scheme propagates traffic announcements along multi-hop paths to ensure that all intermediate nodes remain awake to receive and forward the pending data frames with minimum latency.
* Contact: Vladimir Vukadinovic <vvuk@disneyresearch.com>, Ioannis Glaropoulos <ioannisg@kth.se>
* Author(s): Ioannis Glaropoulos, DRZ
* Affiliation(s): Disney Research Zurich
* License: BSD 3-Clause License http://opensource.org/licenses/BSD-3-Clause
* Location of the code: in this directory
* Contiki version: 2.6
* Intended CALIPSO scenario: Smart Toys
* Intended platforms: Arduino Due with AR9170-based 802.11 radio
* Tested platforms: Arduino Due with FRITZ!WLAN USB Stick N v1
* Tested environment: 

## Description

We implemented the multi-hop power save mode (MH-PSM) for 802.11 [1] as a part of our Contiki80211 module for Contiki. The hardware platform that we used to validate MH-PSM and Contiki80211 consists of an Arduino Due board connected via USB interface to an 802.11n transceiver based on Atheros AR9001U-2NX chipset. The AR9001U-2NX chipset contains an AR9170 MAC/baseband and an AR9104 (dual-band 2\texttimes2) radio chip. Atheros has released the firmware of AR9170 as open source, which helped us write an AR9170 
driver for Contiki. The open source firmware provides a direct access to the lower-MAC program that runs on the AR9170 chip, which greatly simplifies the debugging of the driver and Contiki80211 code.

We fully integrated Contiki80211 (and MH-PSM, as a part of it) into the Contiki NET protocol stack. Contiki NET lacks an IEEE 802.11 library, which had to be implemented. We wrote a lightweight non CPU-dependent Contiki IEEE 802.11 library (NETSTACK_IEEE80211), as part of the Contiki80211. NETSTACK_IEEE80211 library is responsible for the following operations:

* IBSS management operations (scan, join, create, leave)
* Beacon and IBSS parameters configuration 
* PSM and MH-PSM related operations
* Frame generation and parsing

NETSTACK_IEEE80211 includes the IEEE80211 Scheduler, which runs in parallel to the AR9170 Scheduler of the AR9170 Radio Driver.IEEE80211 Scheduler implements standard PSM and MH-PSM functionalities, such as the ATIM packet creation and parsing and the maintenance of the list of awake neighbors. NETSTACK_IEEE80211 also includes the Ieee80211 Framer, which encapsulates IP packets into 802.11 frames and delivers them to the AR9170 Radio Driver. It also parses incoming frames from the underlying driver and delivers them to the Ieee80211 module that is responsible for duplicate frame detection. Ieee80211 is a part of the Contiki NETSTACK_CONF_MAC and serves as an interfacing module between the Contiki80211 and the NETSTACK_CONF_NET layer of the Contiki NET stack. At this level we implemented NullNET, which connects the underlying Contiki80211 implementation with the Contiki uIP stack. 

## CALIPSO Publications

* [1] I. Glaropoulos, S. Mangold, and V. Vukadinovic, “Enhanced IEEE 802.11 Power Saving for Multi-Hop Toy-to-Toy Communication, ” Proc. IEEE Internet of Things (iThings), Beijing, China, Aug. 2013.
