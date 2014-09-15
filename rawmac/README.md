# RAWMAC

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
Optimized Wireless Networks (GROWN 2014), in conjunction with the 10th IEEE International Conference on Wireless and Mobile Computing, Networking and Communica-
tions (WiMob 2014), Larnaca, Cyprus, October 2014.

The code is fully integrated in the calipso-integrated repository.
In order to compile a Contiki application with RAWMAC, you need to activate it in your application project-conf.h.