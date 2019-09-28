# cyberd

cyberd is an init process for HeylelOS.

## Why?

Primarily because as you're certainly aware, systemd is controversial. And for all those reasons,
I decided to replace it. The main reason is because systemd's architecture is against the simple
principle of "each process its task" behind the UNIX philosophy.

Secondly, why not take sysvinit, or another one? Because I don't agree with the notion of runlevels.
A very good idea at the time it was created, but now pretty useless. When runlevels where created,
users chose between a graphical or a terminal based boot.
But now users don't care anymore and just boot in one mode. Or the bootloader does the job.
So this init system just loads daemons and allows users to start, stop, signal and end them.

## Features

cyberd can:
- Start daemons, with command line arguments, custom uid, gid, umask and working directory. At cyberd load, configuration reload or a depending on a previous exit status.
- Stop daemons, by sending them a SIGTERM or a custom signal.
- Reload daemons, by sending a SIGHUP or a custom signal.
- End daemons, by sending them a SIGKILL signal.
- Poweroff, Halt, Reboot, Suspend kernel.
- Create new IPC sockets with less or equal rights than the previous one.

For further details, you can read the manual pages or even available tests for examples of simple daemons configurations.

