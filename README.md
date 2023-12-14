**nmdaemon**

This is an attempt to create a daemon that manages network configuration of FreeBSD.
The daemon should be started as root, it opens a unix socket where clients can connect.
The permissions of the unix socket are managed by `nmdaemon` and provided by it's configuration file.

The daemon can manage:

- some system network parameters
- interfaces
- routes
- wireless networks (using wpa_supplicant)


To communicate with the daemon one should use JSON formatted commands, the daemon answers with JSON formatted data too.
The examples of commands can be found in commands.txt (in doc folder).

The daemon does not use any external tool (ifconfig, route, wpa_cli...) when it is possible, any reconfiguration is done using system calls or sockets.

The current status of commands support can be found in STATUS.md

To build the project, two external libraries are needed:

- sockpp (https://github.com/fpagliughi/sockpp)
- asyncplusplus (https://github.com/Amanieu/asyncplusplus)

Static versions of these libraries must be installed, so CMAKE function find_library can find them by names (*.a).

The `nmdaemon` needs a configuration file, an example is provided as [doc/nmdaemon.conf](https://github.com/Peter2121/nmdaemon/blob/master/doc/nmdaemon.conf).
The full name of the configuration file to use can be passed as a command-line argument:
`sudo ./nmdaemon /home/user1/nmdaemon.conf`

If the configuration file is not provided - some default configuration values are used.

The `nmdaemon` can be (manually) installed as `/usr/local/bin/nmdaemon` and started as FreeBSD service with rc.d script provided as [rc.d/nmdaemon](https://github.com/Peter2121/nmdaemon/blob/master/rc.d/nmdaemon)

Some functional tests of `nmdaemon` using `nc` tool are provided in [tests](https://github.com/Peter2121/nmdaemon/tree/master/tests)
Attention, these tests can change network configuration of the host where they are launched (use with care).

The API il (almost) stable, the FreeBSD port will be created shortly after the release of 0.1 version.

