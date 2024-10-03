# NAME

cyberd - Cyberd init daemons configuration files

# DESCRIPTION

Cyberd daemons configurations file follow an INI-like syntax.

They are separated into sections. Three sections are available: **general**, **environment** and **start**.

All file starts in the **general** section by default, you can enter a section by writing **[\<section name\>]** in a line. All following lines will concern this section until another section is specified.

Each line which is not a section specification is either a scalar value, or an associative value. Scalar values consist of any line without an '=' character. Whereas assocative values are in the form **\<identifier\>=\<value\>**.

Comments can start anywhere on a line with the '#' character, ending on the end of the line and not impacting the content. Also note associative values, scalar values and section names are trimmed for spaces.

# GENERAL
- **arguments=\<argument list\>**: Argument list of the process, if not present, the first argument will be the daemon's name. The list is truncated according to spaces.
- **group=\<name|id\>**: Group name/id of the daemon, defaults to 0.
- **nosid**: By default, **setsid(2)** is called for each daemon, setting nosid keeps init's session.
- **path=\<absolute path\>**: Path to the daemon executable file, MUST be specified.
- **priority=\<priority\>**: Priority of the newly created daemon, must be between -20 and 19 inclusive, defaults to 0.
- **sigfinish=\<signal name|signal number\>**: Signal used to terminate the daemon, defaults to SIGTERM.
- **sigreload=\<signal name|signal number\>**: Signal used to reload the daemon's configuration, defaults to SIGHUP.
- **stdin=\<absolute path\>**: Absolute path of a file opened read-only as stdin, defaults to /dev/null.
- **stdout=\<absolute path\>**: Absolute path of a file opened write-only (no truncate, append nor creat) as stdout, defaults to /dev/null.
- **stderr=\<absolute path\>**: Absolute path of a file opened write-only (no truncate, append nor creat) as stderr, if not specified, stderr is a dup(2) of stdout.
- **umask=\<octal numeric value\>**: umask of the daemon, defaults to cyberd's one (022).
- **user=\<name|id\>**: User name/id of the daemon, defaults to 0.
- **workdir=\<absolute path\>**: Where the daemon will be executed (chdir()'d), defaults to /.

# ENVIRONMENT
The environment section is composed of associated values, any scalar value will be expanded to an associative value with itself, and will compose the daemon's environment variables.

# START
- **any exit**: Start the daemon when the daemon exited in any way.
- **dumped**: Start the daemon when the daemon dumped core.
- **exit failure**: Start the daemon when the daemon exited with **_exit(2)** with an exit code not equal to **0**.
- **exit success**: Start the daemon when the daemon exited with **_exit(2)** with exit code **0**.
- **exit**: Start the daemon when the daemon exited with **_exit(2)**.
- **killed**: Start the daemon when the daemon was killed by a signal.
- **load**: Start the daemon when cyberd is starting.
- **reload**: Start the daemon when cyberd is reloading its configuration.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
cyberctl(1), cyberd(8), isspace(3), chdir(2), umask(2), signal(7).
