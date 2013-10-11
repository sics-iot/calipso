# SVELTE: A real-time IDS for the Internet of Things

## About

* Short description: A new IDS for the IoT: Design, Implementation, and Evaluation in the Contiki OS.
* Contact: Shahid Raza <shahid@sics.se>
* Author(s): "linus@sics.se Wallgren" <linus@sics.se>, Shahid Raza <shahid@sics.se>
* Affiliation(s): SICS Swedish ICT, Stockholm
* License: BSD http://www.opensource.org/licenses/bsd-license.php
* Location of the code: https://github.com/ecksun/Contiki-IDS/tree/IDS
* Contiki version: https://github.com/ecksun/Contiki-IDS/tree/IDS
* Intended CALIPSO scenario: Any using RPL and requiring IDS
* Intended platforms: any with CC2420
* Tested platforms: Tmote Sky
* Tested environment: e.g. COOJA

## Description

This repository contains the source code of SVELTE, the details of which are provided ”SVELTE: Real-time Intrusion Detection in the Internet of Things” published at Ad Hoc Networks, El- sevier, 2013. We implement SVELTE and the mini-firewall in the Contiki OS. Contiki has a well tested implementation of RPL (ContikiRPL). As SVELTE is primarily designed to detect routing attacks we make use of the RPL implementation in the Contiki operating system to develop the 6Mapper, the firewall, and the intrusion detection modules. The RPL implemen- tation in Contiki utilizes in-network routing where each node keeps track of all its descendants. We borrow this feature to detect which nodes should be available in the network. To provide IP communication in 6LoWPAN we use ?IP, an IP stack in Contiki, and SICSLoWPAN - the Contiki implementation of 6LoWPAN header compression. We also implement the sinkhole and selective forwarding attacks against RPL to evaluate SVELTE.

## CALIPSO Publications

[1] Shahid Raza, Linus Wallgren, Thiemo Voigt. SVELTE: Real-time Intrusion Detection in the Internet of Things. Ad Hoc Networks (Elsevier), 11(8), 2661-2674, November, 2013.
[2] Linus Wallgren, Shahid Raza, and Thiemo Voigt. Routing Attacks and Countermeasures in the RPL-Based Internet of Things. International Journal of Distributed Sensor Networks, vol. 2013, Article ID 794326, 11 pages, 2013. doi:10.1155/2013/794326. (Impact Factor:  0.727)

## Other Publications
