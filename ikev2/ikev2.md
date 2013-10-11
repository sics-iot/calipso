# IKEv2: Internet Key Exchange version 2 for Constrained Devices

## About

* Short description: Implementation of the Internet Key Exchange version 2 (IKEv2) protocol for constrained devices in the Contiki OS.
* Contact: Shahid Raza <shahid@sics.se>
* Author(s): Vilhelm Jutvik <ville@sics.se>, Kasun Hewage <kasun.ch@gmail.com>, Simon Duquennoy <simonduq@sics.se>, Dogan Yazar <doganyazar@yshoo.com>
* Affiliation(s): SICS Swedish ICT, Stockholm, Uppsala University
* License: BSD http://www.opensource.org/licenses/bsd-license.php
* Location of the code: https://github.com/vjutvik/Contiki-IPsec
* Contiki version: https://github.com/vjutvik/Contiki-IPsec
* Intended CALIPSO scenario: Any requiring IPv6 and Security
* Intended platforms: any with CC2420
* Tested platforms: WiSMote
* Tested environment: e.g. COOJA, Two nodes network of WiSMote

## Description

We provide an open source implementation of IKEv2 for the Contiki OS, and integrate it with the our existing IPsec implementation. It’s a basic implementation of IPsec, the network layer security extension of IPv6, and a daemon for automatically negotiating secure connections with other hosts - IKEv2.Major features implemented:• State machine for responder• State machine for initiator• ECC DH (only DH group 25)• Authentication by means of pre-shared key• Traffic selector negotiation (note: this is only a simplification of section 2.9’s algorithm) • SPD interface that allows multiple IKE SA offers• SK payload: Encryption and integrity is the same as in IPsec (it’s a common interface) • Multiple concurrent sessions supported• Multiple child SAs per IKE SAMajor features not implemented• Cookie handling (the code is there, but it’s not tested) • Only tunnel mode supported• EAP• No NAT support whatsoever, anywhere?• No IPcomp support• Only IDs of type ID RFC822 ADDR (e-mail address) are supported • No support for Certificates as IDs, nor for authentication

## CALIPSO Publications

## Other Publications
