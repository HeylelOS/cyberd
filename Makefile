CC=clang
CFLAGS=-O3 -pedantic -Wall
BUILDDIRS=build/ build/bin/
INIT_DAEMONS=$(wildcard src/init/daemons/*.c)
INIT_SCHEDULER=$(wildcard src/init/scheduler/*.c)
INIT_CORE=$(wildcard src/init/*.c)
INIT_BIN=build/bin/cyberd

.PHONY: all clean

all: $(BUILDDIRS) $(INIT_BIN)

clean:
	rm -rf build/

$(BUILDDIRS):
	mkdir $@

$(INIT_BIN): $(INIT_CORE) $(INIT_SCHEDULER) $(INIT_DAEMONS)
	$(CC) $(CFLAGS) -o $@ $^

