# rpl-routing-metrics

## About

* Short description: a routing metric which minimizes the delay towards the DAG root, assuming that nodes run with very low duty cycles (e.g., under 1%) at the MAC layer.
* Contact: Pietro Gonizzi <pietro.gonizzi@studenti.unipr.it>
* Author(s): Riccardo Monica, Pietro Gonizzi, University of Parma, Italy
* Affiliation(s): Department of Information Engineering, University of Parma
* License: none
* Location of the code: https://github.com/pietrogonizzi/rpl-routing-metrics
* Contiki version: 2.6
* Intended CALIPSO scenario: Smart Parking, Smart Surveillance, Smart Toys
* Intended platforms: Tmote Sky
* Tested platforms: Tmote Sky, wsn430
* Tested environment: COOJA, Senslab testbed

## Description

This library is a fork of Contiki that contains an implementation of a RPL routing metric aimed at the minimization of the average delay towards the DAG root,
assuming that nodes run with very low duty cycles (e.g., under 1%) at the MAC layer. We refer to the new RPL
metric as Averaged Delay (AVG_DEL) metric. RPL is the IETF standard for IPv6 routing in low-power WSNs.

Moreover, we enhance ContikiMAC to support different sleeping periods of the nodes.

Change git branch to visualize the code.
The core implementation of the metric is in core/net/rpl/rpl-of-etx.c

This work has been published in [1].

## CALIPSO Publications

* [1] P. Gonizzi, R. Monica, G. Ferrari, Design and Evaluation of a Delay-Efficient RPl Routing Metric. In Proceedings of the 9th International Wireless Communications & Mobile Computing Conference (IWCMC 2013), Cagliari, Italy, July 2013.

