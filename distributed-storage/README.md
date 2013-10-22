# Distributed data storage

## About

* Short description: a redundant distributed data storage and retrieval mechanism on top of RPL routing for a robust and efficient data placement
* Contact: Pietro Gonizzi <pietro.gonizzi@studenti.unipr.it>
* Author(s): Pietro Gonizzi, University of Parma, Italy
* Affiliation(s): Department of Information Engineering, University of Parma
* License: none
* Location of the code: in this directory
* Contiki version: 2.6
* Intended CALIPSO scenario: Smart Parking, Smart Surveillance
* Intended platforms: Tmote Sky
* Tested platforms: Tmote Sky, wsn430
* Tested environment: COOJA, Senslab testbed

## Description

This library contains an implementation of a redundant distributed data storage and retrieval mechanism to increase the resilience and storage capacity
of a RPL-based WSN against local memory shortage. RPL is the IETF standard for IPv6 routing in low-power WSNs.

Within this implementation, sensor nodes keep on collecting data (acquired with a given
sensing rate). In order to prevent data losses, data is replicated in several nodes (possibly including the generating node). This
consists in copying and distributing replicas of the same data to other nodes with some available memory space.
Information about memory availability is periodically broadcasted, by each node, to all direct neighbors.
Data retrieval is performed by an external agent that periodically connects to the DAG root and gathers all the data from the WSN.

This work has been published in [1,2].

## CALIPSO Publications

* [1] P. Gonizzi, G. Ferrari, P. Medagliani and J. Leguay, Data Storage and Retrieval with RPL Routing. In Proceedings of the 9th International Wireless Communications & Mobile Computing Conference (IWCMC 2013), Cagliari, Italy, July 2013.
* [2] P. Gonizzi, G. Ferrari, J. Leguay and P. Medagliani, , "Distributed Data Storage and Retrieval Schemes in RPL/IPv6-based networks'' chapter contribution in ``Wireless Sensor Networks: From Theory to Applications''. edited by I. M. M. El Emary and S. Ramakrishnan CRC Press, Taylor and Francis Group, September 2013.
