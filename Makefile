############
# Makefile #
############

# C source files
SOURCES = kbhit.c serial.c vbus.c sqlite.c mqtt.c config.c homeassistant.c main.c

# Optimization
OPT = -O3 -flto

# For Debugging
#OPT = -g

TARGET = vbus-collector

#===================================================================================

GIT_VERSION := "$(shell git describe --long --always --tags)"

CC = gcc
CFLAGS = -std=gnu11 $(OPT) -c -Wall -Ipaho.mqtt.c/src/ -D__SQLITE__ -D__JSON__ -DGIT_VERSION=\"$(GIT_VERSION)\"
LDFLAGS = -LcJSON/build/ -Lpaho.mqtt.c/build/src/ $(OPT) -fuse-linker-plugin -lcurl -lsqlite3 -l:libpaho-mqtt3c.a -l:libcjson.a -lpthread
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

REMOVE    = rm -f
REMOVEDIR = rm -rf
CREATEDIR = mkdir -p
GET_VERSION = git describe --tags --long

# Object files directory
OBJDIR = obj

#===================================================================================

all: createdirs $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

createdirs:
	@$(CREATEDIR) $(OBJDIR)

clean:
	$(REMOVEDIR) $(OBJDIR)
	$(REMOVE) $(TARGET)
