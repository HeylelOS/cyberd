CC=clang
CFLAGS=-g -pedantic -Wall -fPIC
LDFLAGS=
BUILDDIRS=build/ build/bin/
INIT_SRC=$(wildcard src/*.c)\
	$(wildcard src/daemon_conf/*.c)\
	$(wildcard src/fde/*.c)
INIT_BIN=build/bin/cyberd

INITCTL_SRC=$(wildcard ctl/*.c)
INITCTL_BIN=build/bin/cyberctl

.PHONY: all clean

all: $(BUILDDIRS) $(INIT_BIN) $(INITCTL_BIN)

clean:
	rm -rf build/

$(BUILDDIRS):
	mkdir $@

$(INIT_BIN): $(INIT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(INITCTL_BIN): $(INITCTL_SRC)
	$(CC) $(CFLAGS) -o $@ $^

