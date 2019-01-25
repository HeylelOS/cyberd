# Inter-process control of cyberd

This document describes the format used to communicate with a cyberd instance.
Through the use of dedicated control sockets, the program requests actions to cyberd.
Each socket has an associated set of capabilities. Those determine which commands
cyberd will accept from the socket.

Each command goes through those stages in cyberd
parsed -> checked -> scheduled -> executed

The parse section is done by automaton.c from an fde connection.
The check section is done by fde.c when a command has been fully parsed
The scheduling is done by fde.c to scheduler.c
The execution is done by main.c

So if you want to add a command, take care to update each step.

## Generic command format

Commands are ASCII-coded.

Each command is a line, ended by the ASCII NUL character.
They obey the following format:

<command> <when> <arguments...>

Where:
<command>: One of the lowercased identifier specified in the next sections.
<when>: One digit-composed or empty word, interpreted as a unix timestamp, corresponds to universal UTC time.
an overflow is considered an invalid timestamp.
<arguments...>: Command defined value/values.
Separation is done by only one whitespace.

<daemon name> and <acceptor name> refers to filenames
not beginning with a '.', so not composed by any '\0' nor '/'.
The filename max size also applies.

## Daemons related commands

dstt <when> <daemon name> : Schedule start of a daemon
dstp <when> <daemon name> : Schedule stop of a daemon
drld <when> <daemon name> : Schedule reload of a daemon
dend <when> <daemon name> : Schedule end of a daemon

## System related commands

shlt <when> : Halt system
srbt <when> : Reboot system
sslp <when> : Put system to sleep

## Miscellaneous commands

cctl <when> <removed capabilities> <acceptor name> : Create a new control socket
	<removed capabilities> : Zero or more of the previously defined command names specifying
		commands the new socket cannot execute.

