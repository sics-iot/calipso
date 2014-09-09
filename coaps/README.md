# CoAPs -- Secured CoAP through DTLS

## About

* Short description: Implementation of CoAPs (raw public key) based on TinyDTLS
* Contact: Denis Sitenkov <sitenkov@kth.se>
* Author(s): Denis Sitenkov, SICS
* Affiliation(s): SICS Swedish ICT AB
* License: BSD http://www.opensource.org/licenses/bsd-license.php
* Location of the code: https://github.com/sics-iot/calipso-integrated/pull/15 (or commit 6cf33e6f1456da0bb855a4997805b7f09f5e315b in https://github.com/sics-iot/calipso-integrated)
* Contiki version: Contiki 2.7
* Intended CALIPSO scenario: Smart Toys
* Intended platforms: Arduino Due with low-power WiFi dongle
* Tested platforms: Arduino Due with low-power WiFi dongle and WisMote

## Description

Contki Implementation of CoAPs based on TinyDTLS. Supports raw public keys with ECC and Diffie-Hellman.
A full performance evaluation was performed on the WisMote platform [1].
Fully tested on the intended platform for Smart Toys (Arduino Due with low-power WiFi dongle)

## CALIPSO Publications

* [1] Shahid Raza, Hossein Shafagh, Kasun Hewage, René Hummen, Thiemo Voigt. Lithe: Lightweight Secure CoAP for the Internet of Things. IEEE Sensors Journal, 13(10), 3711-3720, October 2013
