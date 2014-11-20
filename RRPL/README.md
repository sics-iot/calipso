# RRPL -- Reactive RPL

## About

* Short description: Implementation of the RRPL protocol
* Contact: Martin Heusse <martin.heusse@imag.fr>
* Author(s): Martin Heusse, CNRS
* Affiliation(s): CNRS
* License: BSD http://www.opensource.org/licenses/bsd-license.php
* Location of the code: https://github.com/martin-heusse/RRPL and in calipso-integrated, c.f. PR https://github.com/sics-iot/calipso-integrated/pull/26
* Contiki version: Contiki 2.7
* Intended CALIPSO scenario: Smart Toys and Smart Parking
* Intended platforms: Any
* Tested platforms: Arduino Due with low-power WiFi dongle and Sky

## Description

This is an implementation of RRPL [1], derived from the AODV of contiki.
It adds the construction of a collection tree by means of sending OPT messages that play a similar role to the DIO messages of RPL. 

## CALIPSO Publications

* [1] Chi-Anh La, Martin Heusse and Andrzej Duda. Link Reversal and Reactive Routing in Low Power and Lossy Networks. In Proceedings of PIMRC’13 (24 th Annual IEEE International Symposium on Personal, Indoor and Mobile Radio Communications), 8-11 September 2013, London, UK.
