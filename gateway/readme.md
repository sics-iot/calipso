# Calipso Gateway - A HTTP/CoAP proxy and cache
 
## About

* Short description: A proxy, cache and gateway for CoAP resources.
* Contact: name <remy.leone@telecom-paristech.fr>
* Author(s): Rémy Léone, Paolo Medagliani 
* Affiliation(s): Thales Communications & Security, LINCS
* License: BSD licence
* Location of the code: [Github](//github.com/sieben/calipso-gateway) 
* Contiki version: Any version implementing a CoAP version compatible
  with Californium. 
* Intended CALIPSO scenario: Any requiring CoAP/HTTP requests.
* Intended platforms: Tmote Sky, CoAP implementations (coap.me)
* Tested platforms: Tmote Sky, coap.me, Californium
* Tested environment: e.g. COOJA, coap.me

## Description

This gateway offers an interface between HTTP requests and CoAP
resources.  When an HTTP requests is received by the gateway, it's
translated into a CoAP request. Then the gateway is going to lookup in
the cache if the information is already available. If it is then the
cached version is sent back as an answer and a communication to the
wireless sensor network is avoided. If a cached version of the
information is missing, a CoAP request is issued. When the result of the
CoAP request is available, it's stored in the cache for later use and
delivered to the client.

We use Java to generate CoAP requests and receive HTTP requests. We use
redis as a back-end to our cache.

To manage feeds of information that are managed in CoAP by using
OBSERVE, we use the pub sub feature of redis.

Because there is bindings for redis in various language, it's easy to
use redis as a data store and plug new service around it. 

## CALIPSO Publications

* [1] Optimizing QoS in Wireless Sensors Networks Using a Caching
  Platform Remy Leone, Paolo Medagliani and Jéremie Leguay

* [2] Optimisation de la qualité de service par l’utilisation de mémoire
  cache
