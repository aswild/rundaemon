# Makefile for rundaemon

VERSION = 1.0

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CC        ?= gcc
CFLAGS    ?= -g -O2
ALL_CFLAGS = -Wall -Wextra -Werror -DVERSION_STR=\"$(VERSION)\" $(CFLAGS)

TARGET = rundaemon
SOURCE = rundaemon.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) -o $(TARGET) $^

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET)

.PHONY: all install clean
