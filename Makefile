DEFINES += PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = packet_bridge
TARGET = cc2530dk

CONTIKI_WITH_RIME = 1

# For CC2530/CC2531, enable banking
HAVE_BANKING = 1

all: $(CONTIKI_PROJECT)

CONTIKI = ../contiki
include $(CONTIKI)/Makefile.include

