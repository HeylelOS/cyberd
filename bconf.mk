CFLAGS+=-std=c2x
CPPFLAGS+=-D_DEFAULT_SOURCE

cyberd-objs:= \
	src/cyberd/configuration.o \
	src/cyberd/daemon.o \
	src/cyberd/daemon_conf.o \
	src/cyberd/main.o \
	src/cyberd/signals.o \
	src/cyberd/socket_connection_node.o \
	src/cyberd/socket_endpoint_node.o \
	src/cyberd/socket_node.o \
	src/cyberd/socket_switch.o \
	src/cyberd/spawns.o \
	src/cyberd/tree.o

cyberctl-objs:=src/cyberctl/main.o

src/cyberd/daemon.o: CPPFLAGS+= \
	-DCONFIG_DAEMON_DEFAULT_WORKDIR='"$(CONFIG_DAEMON_DEFAULT_WORKDIR)"' \
	-DCONFIG_DAEMON_DEV_NULL='"$(CONFIG_DAEMON_DEV_NULL)"'
ifneq ($(CONFIG_DAEMON_PROC_SELF_FD),)
src/cyberd/daemon.o: CPPFLAGS+=-DCONFIG_DAEMON_PROC_SELF_FD=\"$(CONFIG_DAEMON_PROC_SELF_FD)\"
endif

src/cyberd/daemon_conf.o: CPPFLAGS+= \
	-DCONFIG_DAEMON_CONF_DEFAULT_UMASK='0$(CONFIG_DAEMON_CONF_DEFAULT_UMASK)' \
	-DCONFIG_DAEMON_CONF_MAX_UID='$(CONFIG_DAEMON_CONF_MAX_UID)' \
	-DCONFIG_DAEMON_CONF_MAX_GID='$(CONFIG_DAEMON_CONF_MAX_GID)'
ifneq ($(CONFIG_DAEMON_CONF_HAS_RTSIG),)
src/cyberd/daemon_conf.o: CPPFLAGS+=-DCONFIG_DAEMON_CONF_HAS_RTSIG
endif

src/cyberd/main.o: CPPFLAGS+= \
	-DCONFIG_CONFIGURATION_PATH='"$(CONFIG_CONFIGURATION_PATH)"' \
	-DCONFIG_SOCKET_ENDPOINTS_PATH='"$(CONFIG_SOCKET_ENDPOINTS_PATH)"' \
	-DCONFIG_SOCKET_ENDPOINTS_ROOT='"$(CONFIG_SOCKET_ENDPOINTS_ROOT)"'
ifneq ($(CONFIG_RC_PATH),)
src/cyberd/main.o: CPPFLAGS+=-DCONFIG_RC_PATH=\"$(CONFIG_RC_PATH)\"
endif

src/cyberd/socket_connection_node.o: CPPFLAGS+= \
	-DCONFIG_SOCKET_CONNECTIONS_BUFFER_SIZE='$(CONFIG_SOCKET_CONNECTIONS_BUFFER_SIZE)'
src/cyberd/socket_endpoint_node.o: CPPFLAGS+= \
	-DCONFIG_SOCKET_ENDPOINTS_MAX_CONNECTIONS='$(CONFIG_SOCKET_ENDPOINTS_MAX_CONNECTIONS)'

src/cyberctl/main.o: CPPFLAGS+= -I$(srcdir)/src/cyberd \
	-DCONFIG_SOCKET_ENDPOINTS_PATH='"$(CONFIG_SOCKET_ENDPOINTS_PATH)"' \
	-DCONFIG_SOCKET_ENDPOINTS_ROOT='"$(CONFIG_SOCKET_ENDPOINTS_ROOT)"'

cyberd: $(cyberd-objs)
cyberctl: $(cyberctl-objs)

host-libexec+=cyberd
host-sbin+=cyberctl
clean-up+=$(host-sbin) $(host-libexec) $(cyberd-objs) $(cyberctl-objs)

####################
# Functional tests #
####################

ifeq ($(shell pkg-config --exists cover && echo $$?),0)
TEST_CC?=$(CC)
TEST_CFLAGS?=$(CFLAGS) $(shell pkg-config --cflags cover) -I$(srcdir)/src $(CPPFLAGS) $(LDFLAGS)
TEST_LDLIBS?=$(LDLIBS) $(shell pkg-config --libs cover)

tests:=test/cyberd-tree
checks:=$(tests:test/%=check-%)

$(tests): %: %.c
	$(v-e) TEST_CC $@
	$(v-a) $(MKDIR) $(@D) && $(TEST_CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDLIBS)

$(checks): check-%: test/%
	$(v-e) CHECK $@
	$(v-a) $< -pretty -

.PHONY: check $(checks)

check: $(checks)

clean-up+=$(tests)
endif
