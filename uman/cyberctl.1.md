# cyberctl 1 2019-10-20 HeylelOS

## NAME
cyberctl - Send command to a cyberd init instance.

## SYNOPSIS
- **cyberctl** [-s socket] [-n] [-t when] \<start|stop|reload|end\> daemon...
- **cyberctl** [-s socket] [-n] [-t when] \<poweroff|halt|reboot|suspend\>
- **cyberctl** [-s socket] -c \<name\> [start|stop|reload|end|poweroff|halt|reboot|suspend|cctl]...

## DESCRIPTION
This command line interface is meant to execute commands to a cyberd instance. You can start, stop, reload or force-end a daemon. Depending on supported features, you can also poweroff, halt, reboot or suspend your system. Finally you can create a new socket to communicate witch cyberd, and remove supported commands for this new socket. This allows creations of less-priviliged sockets to restrict access only to needed features of cyberd.

## OPTIONS
- -s socket : Name of the socket to connect to.
- -n : Executes the command relative to the current time, not since epoch.
- -t \<when\> : The when specifies the unix timestamp at which the command may be executed.
- -c \<name\> : Name of the new socket to create.

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
cyberd(5), cyberd(8).

