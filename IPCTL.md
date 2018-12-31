# Inter-process control of cyberd

This document describes the format used to communicate with a cyberd instance.
Through the use of dedicated sockets, the program requests actions to cyberd.
Each socket has an associated set of capabilities. Those determine which command can be sent
by each socket.

## Generic command format

Commands are UTF8-coded.

Each command is a line, ended by the ASCII newline character.
They obey the following format:

<command> <when> <arguments...>

Where:
<command>: One lowercased identifier specified in the next sections.
<when>: One digit-composed word, interpreted as a unix timestamp, corresponds to system time.
<arguments...>: Command defined value/values.
Separation is done by only one whitespace.

## Daemons related commands

dstt <when> <daemon name> : Schedule start of a daemon
dstp <when> <daemon name> : Schedule stop of a daemon
drld <when> <daemon name> : Schedule reload of a daemon
dend <when> <daemon name> : Schedule end of a daemon

## Shutdown related commands

shlt <when> : Halt system
srbt <when> : Reboot system
sslp <when> : Put system to sleep

## Miscellaneous commands

macc <when> <acceptor name> <removed capabilities> : Create a new socket to communicate
	<removed capabilities> : Zero or more of the previously defined command names specifying
		commands the new socket cannot execute.

