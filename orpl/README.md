# ORPL -- Opportunistic RPL

## About

* Short description: source code of ORPL, an opportunistic version of the RPL routing protocol
* Contact: Simon Duquennoy <simonduq@sics.se>
* Author(s): Simon Duquennoy, SICS
* Affiliation(s): SICS Swedish ICT AB
* License: BSD http://www.opensource.org/licenses/bsd-license.php
* Location of the code: https://github.com/simonduq/orpl
* Contiki version: https://github.com/simonduq/orpl/tree/master/contiki
* Intended CALIPSO scenario: any requiring RPL with low delay and low energy
* Intended platforms: any with CC2420
* Tested platforms: Tmote Sky
* Tested environment: Cooja+MSPSim, Indriya testbed, TWIST testbed

## Description

This repository contains the source code of ORPL, used for the experiments in our paper "Let the Tree Bloom: Scalable Opportunistic Routing with ORPL" published at ACM SenSys 2013 [1].

RPL is a standard routing protocol for low-power IPv6 networks. In the context of 6LoWPAN and 802.15.4, we propose an opportunistic extension of RPL, called ORPL [1,2], that supports on-demand any-to-any traffic in duty-cycled networks. ORPL exploits RPL's rooted topology on top of which it performs multipath routing. Rather than selecting a specific next hop, ORPL nodes anycast to any potential forwarder. This makes ORPL highly agile, resilient to interference, and helps keep the latency low in multihop communication. ORPL scales to large networks by basing the routing decision on routing sets rather than traditional routing tables. Routing sets may utilize Bloo filters for a compact representation and inexpensive propagation of routing states.

The design of ORPL is partly based on our previous work on ORW [4] and on link estimation in dense networks [3].

## Publications

1. Simon Duquennoy, Olaf Landsiedel, and Thiemo Voigt. Let the Tree Bloom: Scalable Opportunistic Routing with ORPL. In Proceedings of the International Conference on Embedded Networked Sensor Systems (ACM SenSys 2013), Rome, Italy, November 2013.
1. Simon Duquennoy and Olaf Landsiedel. Poster Abstract: Opportunistic RPL. In EWSN'13: Proceedings of the 10th European Conference on Wireless Sensor Networks, Ghent, Belgium, February 2013. Poster Abstract.
1. Sébastien Dawans and Simon Duquennoy. On Link Estimation in Dense RPL Deployments. In Proceedings of the International Workshop on Practical Issues in Building Sensor Network Applications (IEEE SenseApp 2012), Clearwater, FL, USA, October 2012.
1. Olaf Landsiedel, Euhanna Ghadimi, Simon Duquennoy, and Mikael Johansson. Low Power, Low Delay: Opportunistic Routing meets Duty Cycling. In Proceedings of the International Conference on Information Processing in Sensor Networks (ACM IPSN 2012), Beijing, China, April 2012.
