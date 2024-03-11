############
# Makefile #
############

# C source files
SOURCES = kbhit.c serial.c vbus.c sqlite.c mqtt.c config.c homeassistant.c main.c

# Optimization
OPT = -O3 -flto
OPT_LINUX = -fwhole-program

# For Debugging
#OPT = -g

TARGET = vbus-collector

#===================================================================================

GIT_VERSION := "$(shell git describe --long --always --tags)"
ROOT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	OPT += $(OPT_LINUX)
endif

CC = gcc
CFLAGS = -std=gnu11 $(OPT) -c -Wall -Ipaho.mqtt.c/src/ -D__SQLITE__ -D__JSON__ -DGIT_VERSION=\"$(GIT_VERSION)\"
DYNAMIC_LIBS = -lcurl -lpthread -lm -lsqlite3
STATIC_LIBS = $(ROOT_DIR)paho.mqtt.c/build/src/libpaho-mqtt3c.a $(ROOT_DIR)cJSON/build/libcjson.a
LDFLAGS = $(OPT) $(DYNAMIC_LIBS) $(STATIC_LIBS)
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
