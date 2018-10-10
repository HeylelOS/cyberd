CC=clang
CFLAGS=-O3 -pedantic -Wall -fPIC
BUILDDIRS=build/ build/bin/ build/lib/

INIT_SCHEDULER=$(wildcard sources/init/scheduler/*.c)
INIT_NETWORKER=$(wildcard sources/init/networker/*.c)
INIT_DAEMONS=$(wildcard sources/init/daemons/*.c)
INIT_SPAWNS=$(wildcard sources/init/spawns/*.c)
INIT_CORE=$(wildcard sources/init/*.c)
INIT_BIN=build/bin/cyberd

LIB_CORE=$(wildcard sources/lib/*.c)
LIB_BIN=build/lib/libcyberctl.so

CTL_CORE=$(wildcard sources/ctl/*.c)
CTL_BIN=build/bin/cyberctl

.PHONY: all clean

all: $(BUILDDIRS) $(INIT_BIN) $(CTL_BIN)

clean:
	rm -rf build/

$(BUILDDIRS):
	mkdir $@

$(INIT_BIN): $(INIT_CORE) $(INIT_SCHEDULER) $(INIT_NETWORKER) $(INIT_DAEMONS) $(INIT_SPAWNS)
	$(CC) $(CFLAGS) -o $@ $^

$(LIB_BIN): $(LIB_CORE)
	$(CC) $(CFLAGS) -shared -o $@ $^

$(CTL_BIN): $(CTL_CORE) $(LIB_BIN)
	$(CC) $(CFLAGS) -o $@ $^ -Isources/lib/ -Lbuild/lib/ -lcyberctl

