############
# Makefile #
############

# C source files
SOURCES = kbhit.c serial.c vbus.c sqlite.c mqtt.c main.c

# Optimization
OPT = -O3 -flto

TARGET = vbus-collector

#===================================================================================

CC = gcc
CFLAGS = -std=gnu99 $(OPT) -c -Wall -Ipaho.mqtt.c/src/ -D__SQLITE__
LDFLAGS = -Lpaho.mqtt.c/build/src/ $(OPT) -fuse-linker-plugin -lsqlite3 -l:libpaho-mqtt3c.a -lpthread
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

REMOVE    = rm -f
REMOVEDIR = rm -rf
CREATEDIR = mkdir -p

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
