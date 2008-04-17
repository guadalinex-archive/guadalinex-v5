# PC/SC Lite libraries and headers.
PCSC_CFLAGS ?= `pkg-config libpcsclite --cflags`
PCSC_LDLIBS ?= `pkg-config libpcsclite --libs`

VERSION=`echo \`pwd\` | sed s/.*tools-//`
CFLAGS=-Wall -O2 -g -DVERSION=\"$(VERSION)\" $(PCSC_CFLAGS)
LDLIBS=$(PCSC_LDLIBS)

BIN=getatr
INST_DIR=/usr/bin

all: $(BIN)

getatr: getatr.o

install:
	dh_install $(BIN) $(INST_DIR)

clean:
	rm -f $(BIN) $(BIN).o

.PHONY: clean all install
