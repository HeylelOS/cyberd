# cyberd

cyberd is an init process for HeylelOS.

## Why?

Primarily because alternatives are too complex.
Systemd is a collection of programs and a framework.
It tightly couples its daemon's and interactions with the whole system.
Launchd (from Apple's macOS), provides xinetd-like network features.
Others usually have a dynamic dependency management, or an event based
system which differentiates each one from another.

Secondly, why not take sysvinit, or another one? Because I don't agree with the notion of runlevels.
A very good idea at the time it was created, but now pretty useless. When runlevels where created,
users chose between a graphical or a terminal based boot.
But now users don't care anymore and just boot in one mode. Or the bootloader does the job.
So this init system just loads daemons and allows users to start, stop, signal and end them.

## Features

cyberd can:
- Start daemons, with command line arguments, custom uid, gid, etc... At configuration load, reload or a depending on a previous exit status.
- Stop daemons, by sending them a SIGTERM or a custom signal.
- Reload daemons, by sending a SIGHUP or a custom signal.
- End daemons, by sending them a SIGKILL signal.
- Create custom connection endpoints.
- Poweroff, Halt, Reboot.

For further details, you can read the manual pages or even available examples of simple daemons configurations.

## Configure, build and install

Download and extract a source release tarball.
```
./configure
make
make install
```
