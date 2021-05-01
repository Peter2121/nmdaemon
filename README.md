**nmdaemon**

This is an attempt to create a daemon that manages network configuration of FreeBSD.
The daemon should be started as root, it opens a unix socket where clients can connect.
The daemon can manage:
- interfaces
- routes
- wireless networks (using wpa_supplicant).
- 
To communicate with the daemon one should use JSON formatted commands, the daemon answers with JSON formatted data too.
The examples of commands can be found in commands.txt (in doc folder).

The daemon does not use any external tool (ifconfig, route, wpa_cli...) when it is possible, any reconfiguration is done using system calls or sockets.

Only few commands are supported in the current state, the project is actually on very early stage. The modifications are (still) not written to any file on disk, so they are lost on reboot.

To build the project, an external library sockpp is needed (https://github.com/fpagliughi/sockpp), it should be placed on the same level of the filesystem as nmdaemon directory and built into build and build-debug subdirectories of sockpp.
