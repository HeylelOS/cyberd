# NAME
cyberd - Init process.

# SYNOPSIS
- **cyberd**

# DESCRIPTION
cyberd is an init process meant for UNIX-like operating systems. It is neither a System V-like nor a BSD-like init system. It only supports daemons, and starts them upon configuration-load/reload. Each daemon is associated with a file in the configuration directory. Only one instance of a daemon can run at any time, and through cyberctl(1), you may start stop or end it.

# FEATURES
cyberd was designed to be a daemon-leader as an init process. Its lack of other functionalities is intended by design.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
cyberctl(1), cyberd(5).

