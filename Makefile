CC=clang
CFLAGS=-O3 -pedantic -Wall -fPIC
LDFLAGS=
BUILDDIRS=build/ build/bin/

INIT_SCHEDULER=$(wildcard src/init/scheduler/*.c)
INIT_NETWORKER=$(wildcard src/init/networker/*.c)
INIT_DAEMONS=$(wildcard src/init/daemons/*.c)
INIT_SPAWNS=$(wildcard src/init/spawns/*.c)
INIT_CORE=$(wildcard src/init/*.c)
INIT_BIN=build/bin/cyberd

CTL_CORE=$(wildcard src/ctl/*.c)
CTL_BIN=build/bin/cyberctl

.PHONY: all clean

all: $(BUILDDIRS) $(INIT_BIN) $(CTL_BIN)

clean:
	rm -rf build/

$(BUILDDIRS):
	mkdir $@

$(INIT_BIN): $(INIT_CORE) $(INIT_SCHEDULER) $(INIT_NETWORKER) $(INIT_DAEMONS) $(INIT_SPAWNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CTL_BIN): $(CTL_CORE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

