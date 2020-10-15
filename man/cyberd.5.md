# cyberd 5 2019-10-20 HeylelOS

## NAME
cyberd - Cyberd init daemons configuration files

## DESCRIPTION
cyberd daemons configurations are separated into sections. Three sections are available: **general**, **environment** and **start**. All file starts in the **general** section by default, you can specify a section by writing **@\<section name\>** in a line. All following lines will concern this section until another section is specified. Each line which is not a section specification is either a scalar value, or an associative value. Scalar values consist of any value without an '=' character. Whereas assocative values are in the form **\<identifier\>=\<value\>**.

## GENERAL
- **path=\<absolute path\>**: Path to the daemon executable file, MUST be specified.
- **workdir=\<absolute path\>**: Where the daemon will be executed (chdir()'d). Default none, executed in cyberd's current working directory.
- **user=\<name|id\>**: User name/id of the daemon.
- **group=\<name|id\>**: Group name/id of the daemon.
- **umask=\<octal numeric value\>**: umask of the daemon, default to cyberd's one (022).
- **sigfinish=\<signal name| signal number\>**: Signal used to terminate the daemon.
- **sigreload=\<signal name| signal number\>**: Signal used to reload the daemon's configuration.
- **arguments=\<argument list\>**: Argument list of the process, if not present, the first argument will be the daemon's name. The list is truncated according to spaces.
- **priority=\<priority\>**: Priority of the newly created daemon, must be between -20 and 20, default to 0.

## ENVIRONMENT
The environment section is composed of associated values, any scalar value will be expanded to an associative value with itself, and will compose the daemon's environment variables.

## START
- **load**: Start the daemon when cyberd is starting.
- **reload**: Start the daemon when cyberd is reloading its configuration.
- **any exit**: Start the daemon when the daemon exited in any way.
- **exit**: Start the daemon when the daemon exited with **_exit(2)**.
- **exit success**: Start the daemon when the daemon exited with **_exit(2)** with exit code **0**.
- **exit failure**: Start the daemon when the daemon exited with **_exit(2)** with an exit code not equal to **0**.
- **killed**: Start the daemon when the daemon was killed by a signal.
- **dumped**: Start the daemon when the daemon dumped core.

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
cyberctl(1), cyberd(8), isspace(3).

