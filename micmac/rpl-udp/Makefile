CONTIKI = ../../../contiki
CONTIKI_PROJECT = udp-client udp-server
APPS=servreg-hack
MCMACDIR=..
PROJECTDIRS += $(MCMACDIR) $(MCMACDIR)/timer32bit
TARGET?=sky

WITH_UIP6=1
UIP_CONF_IPV6=1

CFLAGS+= -DUIP_CONF_IPV6_RPL

ifdef WITH_COMPOWER
APPS+=powertrace
CFLAGS+= -DCONTIKIMAC_CONF_COMPOWER=1 -DWITH_COMPOWER=1 -DQUEUEBUF_CONF_NUM=4
endif

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif

ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif

TIMEDATE=`date +%Y%m%d-%H%M`

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#for local version mccontikimac
CONTIKIDIRS += $(MCMACDIR) $(MCMACDIR)/net $(MCMACDIR)/net/mac
CONTIKI_SOURCEFILES += mccontikimac.c mcphase.c csma.c clock.c rtimer-arch.c

CFLAGS+= -iquote -Werror

#messages IPI is 4 seconds
IPI ?= 4
CFLAGS += -DCMD_IPI=$(IPI)

CYCLE_TIME ?= 125
CFLAGS += -DCMD_CYCLE_TIME=$(CYCLE_TIME)

RF_CHANNEL ?= 26
CFLAGS += -DCMD_RF_CHANNEL=$(RF_CHANNEL)

NUM_CHANNELS ?= 4
CFLAGS += -DCMD_NUM_CHANNELS=$(NUM_CHANNELS)

BC_CHANNELS ?= 0
CFLAGS += -DCMD_BC_CHANNELS=$(BC_CHANNELS)

#useful only if one BC channel is used
BC_RF_CHANNEL ?= 20
CFLAGS += -DCMD_BC_RF_CHANNEL=$(BC_RF_CHANNEL)


PAN_ID ?= 0xBBBB
CFLAGS += -DCMD_PAN_ID=$(PAN_ID)

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

cooja:
	ant run -f $(CONTIKI)/tools/cooja/build.xml
	
%.exe: %.sky
	cp $*.sky $*.exe
