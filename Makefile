CC=clang
CFLAGS=-O3 -pedantic -Wall -fPIC
BUILDDIRS=build/ build/bin/ build/lib/

INIT_DAEMONS=$(wildcard src/init/daemons/*.c)
INIT_SCHEDULER=$(wildcard src/init/scheduler/*.c)
INIT_CORE=$(wildcard src/init/*.c)
INIT_BIN=build/bin/cyberd

LIB_CORE=$(wildcard src/lib/*.c)
LIB_BIN=build/lib/libcyberctl.so

CTL_CORE=$(wildcard src/ctl/*.c)
CTL_BIN=build/bin/cyberctl

.PHONY: all clean

all: $(BUILDDIRS) $(INIT_BIN) $(CTL_BIN)

clean:
	rm -rf build/

$(BUILDDIRS):
	mkdir $@

$(INIT_BIN): $(INIT_CORE) $(INIT_SCHEDULER) $(INIT_DAEMONS)
	$(CC) $(CFLAGS) -o $@ $^

$(LIB_BIN): $(LIB_CORE)
	$(CC) $(CFLAGS) -shared -o $@ $^

$(CTL_BIN): $(CTL_CORE) $(LIB_BIN)
	$(CC) $(CFLAGS) -o $@ $^ -Isrc/lib/ -Lbuild/lib/ -lcyberctl
