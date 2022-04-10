# Inter-process communication with cyberd

This document describes the format used to communicate with a cyberd instance.
Through the use of dedicated sockets (called endpoints), the program requests actions to cyberd.
Each endpoint has an associated set of capabilities. Those capabilities determine which commands cyberd will accept from connections made with this endpoint.

## Generic command format

Commands are binary coded, using network byte order (MSB first, Big-Endian).

Commands are succession of components, there exists three kinds of commands: Daemons related, system related and miscellaneous.

Components are of the following:
- \<command\>: One byte unsigned integer, representing the kind of command. In the following section, the value will be represented using an hexadecimal notation.
- \<when\>: Eight bytes integer, interpreted as a unix timestamp, corresponds to universal UTC time.
- \<removed capabilities\>: Eight bytes integer, bitmask of removed capabilities, each \<command\> value is associated with the \<command\>th bit.
- \<name\> refers to a filename not beginning with a `.`, nor composed by any `/`, ended by a zero byte (also referred to as NUL character). The filename max size also applies, to avoid attacks by buffer overflow.

## Daemons related commands

- 0x08 \<when\> \<name\> : Schedule start of a daemon
- 0x09 \<when\> \<name\> : Schedule stop of a daemon
- 0x0A \<when\> \<name\> : Schedule reload of a daemon
- 0x0B \<when\> \<name\> : Schedule end of a daemon

## System related commands

- 0x10 \<when\> : Power off system
- 0x11 \<when\> : Halt system
- 0x12 \<when\> : Reboot system
- 0x13 \<when\> : Suspend system

## Miscellaneous commands

- 0x01 \<removed capabilities\> \<name\> : Create a new endpoint

