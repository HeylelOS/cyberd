# NAME
cyberctl - Send command to a cyberd init instance.

# SYNOPSIS
- **cyberctl** [-s endpoint] [-E] [-t when] \<start|stop|reload|end\> daemon...
- **cyberctl** [-s endpoint] [-E] [-t when] \<poweroff|halt|reboot|suspend\>
- **cyberctl** [-s endpoint] \<create-endpoint\> [start|stop|reload|end|poweroff|halt|reboot|suspend|create-endpoint]...

# DESCRIPTION

This command line interface is meant to execute commands to a cyberd instance.

You can start, stop, reload or force-end a daemon. Depending on supported features, you can also poweroff, halt, reboot or suspend your system.

Finally you can create a new endpoint to communicate with cyberd, and remove supported commands for this new endpoint. This allows creations of less-priviliged endpoints to restrict access only to required features of cyberd.

# OPTIONS
- -s endpoint : Name of the endpoint to connect to.
- -E : Executes the command relative to the epoch, not current time.
- -t \<when\> : The when specifies the unix timestamp at which the command may be executed.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
cyberd(5), cyberd(8).

