# Endpoints

This document describes the format used to communicate with a cyberd instance.
Through the use of dedicated sockets (called endpoints), the program requests actions to cyberd.
Each endpoint has an associated set of capabilities. Those capabilities determine which commands cyberd will accept from connections made with this endpoint.

## Components

Messages are binary coded, and can be composed of the following items:

|   Component    |                             Format                           |
|----------------|--------------------------------------------------------------|
|    Command     |                            One byte                          |
| Capability set |                  Four bytes, MSB (big endian)                |
|     Name       | N bytes, 0 < N < _NAME\_MAX_, no '/', terminated by nul byte |

## Messages

The available messages are the following:

| Message         | Description                     | First component (command) | Second component        | Third component |
|-----------------|---------------------------------|---------------------------|-------------------------|-----------------|
| Create endpoint | Create a new dedicated endpoint |             0             | Restricted capabilities |      Name       |
| Start daemon    | Start a daemon                  |             1             |           Name          |                 |
| Stop daemon     | Stop a daemon                   |             2             |           Name          |                 |
| Reload daemon   | Reload a daemon                 |             3             |           Name          |                 |
| End daemon      | End a daemon                    |             4             |           Name          |                 |
| System poweroff | Poweroff the system             |             5             |           Name          |                 |
| System halt     | Halt the system                 |             6             |                         |                 |
| System reboot   | Reboot the system               |             7             |                         |                 |
| System suspend  | Suspend the system              |             8             |                         |                 |

