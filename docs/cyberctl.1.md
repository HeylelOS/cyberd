# NAME

cyberctl - Send command to a cyberd init instance.

# SYNOPSIS

- **cyberctl** [-s endpoint] \<start|stop|reload|end\> daemon...
- **cyberctl** [-s endpoint] \<poweroff|halt|reboot|suspend\>
- **cyberctl** [-s endpoint] \<create-endpoint\> [start|stop|reload|end|poweroff|halt|reboot|suspend|create-endpoint]...

# DESCRIPTION

You can start, stop, reload or force-end a daemon. Depending on supported features, you can also poweroff, halt, reboot or suspend your system.

Finally you can create a new endpoint to communicate with cyberd, and remove supported commands for this new endpoint. This allows creations of less-priviliged endpoints to restrict access only to required features of cyberd.

# OPTIONS

- -s endpoint : Name of the endpoint to connect to.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
cyberd(5), cyberd(8).
