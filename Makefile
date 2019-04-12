DEFINES += PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = packet_bridge
PROJECT_SOURCEFILES += serial_protocol.c commands.c serialization.c packet_bridge_mac.c
TARGET = cc2530dk

CONTIKI_WITH_RIME = 1

# For CC2530/CC2531, enable banking
HAVE_BANKING = 1

all: $(CONTIKI_PROJECT)

CONTIKI = ../contiki
include $(CONTIKI)/Makefile.include

