CONTIKI = ../../contiki2.7
CONTIKI_PROJECT = example-unicast
MCMACDIR=..
TARGET?=sky

ifdef PERIOD
CFLAGS=-DPERIOD=$(PERIOD)
endif

TIMEDATE=`date +%Y%m%d-%H%M`

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#for local version mccontikimac
CONTIKIDIRS += $(MCMACDIR) $(MCMACDIR)/net $(MCMACDIR)/net/mac
CONTIKI_SOURCEFILES += mccontikimac.c mcphase.c csma.c

CFLAGS+= -iquote -Werror

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


PAN_ID ?= 0xABCD
CFLAGS += -DCMD_PAN_ID=$(PAN_ID)

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

cooja:
	ant run -f $(CONTIKI)/tools/cooja/build.xml 
	
%.exe: %.sky
	cp $*.sky $*.exe
