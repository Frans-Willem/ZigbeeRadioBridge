DEFINES += PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = radio_bridge
PROJECT_SOURCEFILES += serial_protocol.c commands.c serialization.c radio_bridge_rdc.c
TARGET = cc2530dk

CONTIKI_WITH_RIME = 1

# For CC2530/CC2531, enable banking
HAVE_BANKING = 1

all: $(CONTIKI_PROJECT)

CONTIKI = ./3rdparty/contiki
include $(CONTIKI)/Makefile.include

