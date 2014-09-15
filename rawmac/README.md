# Distributed data storage

## About

* Short description: RAWMAC is a cross-layer integration between ContikiMAC and RPL to speed up data collection
* Contact: Pietro Gonizzi <pietro.gonizzi@studenti.unipr.it>
* Author(s): Pietro Gonizzi, University of Parma, Italy
* Affiliation(s): Department of Information Engineering, University of Parma
* License: none
* Location of the code: https://github.com/sics-iot/calipso-integrated
* Contiki version: 2.7
* Intended CALIPSO scenario: Smart Parking
* Intended platforms: any platform with Contiki support
* Tested platforms: Tmote Sky, Zolertia Z1
* Tested environment: COOJA simulator

## Description

We implement and integrate a radio duty cycling technique in Contiki. The technique is named RAWMAC, a Routing Aware Wave-based MAC
protocol for WSNs. RAWMAC is an extension of ContikiMAC which allows nodes of a WSN to create a wave of propagation towards the sink/DAG root, in order to reduce the upward delay.
The wake-up intervals of the nodes, based on the current RPL topology, are dynamically aligned, so that the delay for data collection is minimized.

RAWMAC has been published at the 2014 International Workshop on the GReen
Optimized Wireless Networks (GROWN 2014), in conjunction with the 10th IEEE Inter-
national Conference on Wireless and Mobile Computing, Networking and Communica-
tions (WiMob 2014), Larnaca, Cyprus, October 2014.

## CALIPSO Publications

* [1] P. Gonizzi, G. Ferrari, P. Medagliani and J. Leguay, Data Storage and Retrieval with RPL Routing. In Proceedings of the 9th International Wireless Communications & Mobile Computing Conference (IWCMC 2013), Cagliari, Italy, July 2013.
* [2] P. Gonizzi, G. Ferrari, J. Leguay and P. Medagliani, , "Distributed Data Storage and Retrieval Schemes in RPL/IPv6-based networks'' chapter contribution in ``Wireless Sensor Networks: From Theory to Applications''. edited by I. M. M. El Emary and S. Ramakrishnan CRC Press, Taylor and Francis Group, September 2013.
