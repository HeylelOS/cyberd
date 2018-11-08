CC=clang
CFLAGS=-O3 -pedantic -Wall -fPIC
LDFLAGS=
BUILDDIRS=build/ build/bin/
INIT_SRC=$(wildcard src/*.c)
INIT_DAEMON_CONF_SRC=$(wildcard src/daemon_conf/*.c)
INIT_BIN=build/bin/cyberd

.PHONY: all clean

all: $(BUILDDIRS) $(INIT_BIN)

clean:
	rm -rf build/

$(BUILDDIRS):
	mkdir $@

$(INIT_BIN): $(INIT_SRC) $(INIT_DAEMON_CONF_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

