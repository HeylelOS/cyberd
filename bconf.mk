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

cyberctl-objs:=src/cyberctl.o

src/cyberd/daemon.o: CPPFLAGS+= \
	-DCONFIG_DAEMON_DEFAULT_WORKDIR='"$(CONFIG_DAEMON_DEFAULT_WORKDIR)"' \
	-DCONFIG_DAEMON_DEV_NULL='"$(CONFIG_DAEMON_DEV_NULL)"'

src/cyberd/daemon_conf.o: CPPFLAGS+= \
	-DCONFIG_DAEMON_CONF_DEFAULT_UMASK='0$(CONFIG_DAEMON_CONF_DEFAULT_UMASK)' \
	-DCONFIG_DAEMON_CONF_MAX_UID='$(CONFIG_DAEMON_CONF_MAX_UID)' \
	-DCONFIG_DAEMON_CONF_MAX_GID='$(CONFIG_DAEMON_CONF_MAX_GID)'
ifneq ($(CONFIG_DAEMON_CONF_HAS_RTSIG),)
src/cyberd/daemon_conf.o: CPPFLAGS+=-DCONFIG_DAEMON_CONF_HAS_RTSIG
endif

src/cyberd/main.o: CPPFLAGS+= \
	-DCONFIG_REBOOT_TIMEOUT='$(CONFIG_REBOOT_TIMEOUT)' \
	-DCONFIG_CONFIGURATION_PATH='"$(CONFIG_CONFIGURATION_PATH)"' \
	-DCONFIG_SOCKET_ENDPOINTS_PATH='"$(CONFIG_SOCKET_ENDPOINTS_PATH)"' \
	-DCONFIG_SOCKET_ENDPOINTS_ROOT='"$(CONFIG_SOCKET_ENDPOINTS_ROOT)"'
ifneq ($(CONFIG_RC_PATH),)
src/cyberd/main.o: CPPFLAGS+=-DCONFIG_RC_PATH='"$(CONFIG_RC_PATH)"'
endif

src/cyberd/socket_connection_node.o: CPPFLAGS+= \
	-DCONFIG_SOCKET_CONNECTIONS_BUFFER_SIZE='$(CONFIG_SOCKET_CONNECTIONS_BUFFER_SIZE)'
src/cyberd/socket_endpoint_node.o: CPPFLAGS+= \
	-DCONFIG_SOCKET_ENDPOINTS_MAX_CONNECTIONS='$(CONFIG_SOCKET_ENDPOINTS_MAX_CONNECTIONS)'

src/cyberctl.o: CPPFLAGS+= -I$(srcdir)/src/cyberd \
	-DCONFIG_SOCKET_ENDPOINTS_PATH='"$(CONFIG_SOCKET_ENDPOINTS_PATH)"' \
	-DCONFIG_SOCKET_ENDPOINTS_ROOT='"$(CONFIG_SOCKET_ENDPOINTS_ROOT)"'

cyberd: $(cyberd-objs)
cyberctl: $(cyberctl-objs)

host-libexec+=cyberd
host-sbin+=cyberctl
clean-up+=$(host-sbin) $(host-libexec) $(cyberd-objs) $(cyberctl-objs)

################
# Manual pages #
################

ifneq ($(CONFIG_MANPAGES),)
man1dir:=$(mandir)/man1
man5dir:=$(mandir)/man5
man8dir:=$(mandir)/man8

host-man:=man/cyberctl.1 man/cyberd.5 man/cyberd.8

.PHONY: install-man uninstall-man

install-data: install-man
uninstall-data: uninstall-man
install-man: $(host-man)
	$(v-e) INSTALL $(host-man)
	$(v-a) $(INSTALL) -d -- "$(DESTDIR)$(man1dir)" "$(DESTDIR)$(man5dir)" "$(DESTDIR)$(man8dir)"
	$(v-a) $(INSTALL-DATA) -- $(filter %.1,$^) "$(DESTDIR)$(man1dir)"
	$(v-a) $(INSTALL-DATA) -- $(filter %.5,$^) "$(DESTDIR)$(man5dir)"
	$(v-a) $(INSTALL-DATA) -- $(filter %.8,$^) "$(DESTDIR)$(man8dir)"
uninstall-man:
	$(v-e) UNINSTALL $(host-man)
	$(v-a) $(RM) -- \
		 $(patsubst %.1,"$(DESTDIR)$(man1dir)/%.1",$(filter %.1,$(notdir $(host-man)))) \
		 $(patsubst %.5,"$(DESTDIR)$(man5dir)/%.5",$(filter %.5,$(notdir $(host-man)))) \
		 $(patsubst %.8,"$(DESTDIR)$(man8dir)/%.8",$(filter %.8,$(notdir $(host-man))))
endif

####################
# Functional tests #
####################

ifneq ($(CONFIG_CHECK),)
tests:=test/cyberd-tree

$(tests): %: %.c
	$(v-e) TEST-CC $@
	$(v-a) $(MKDIR) $(@D) && $(CC) -I$(srcdir)/src \
		$(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

checks:=$(tests:test/%=check-%)

.PHONY: tests check $(checks)

$(checks): check-%: test/%
	$(v-e) CHECK $@
	$(v-a) $<

check: $(checks)
tests: $(tests)

clean-up+=$(tests)
endif
