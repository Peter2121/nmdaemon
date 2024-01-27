**nmdaemon**

This is a daemon that manages network configuration of FreeBSD.

The daemon can manage:

- some system network parameters
- interfaces
- routes
- wireless networks (using wpa_supplicant)

`nmdaemon` should be started as root, it opens a unix socket where clients can connect.
The permissions of the unix socket are managed by `nmdaemon` and provided by it's configuration file.

To communicate with `nmdaemon` one should use JSON formatted commands, `nmdaemon` answers with JSON formatted data too.
The syntax and examples of the commands can be found in [commands.md](https://github.com/Peter2121/nmdaemon/blob/master/doc/commands.md).

`nmdaemon` does not use any external tool (ifconfig, route, wpa_cli...) when it is possible, any reconfiguration is done using system calls or sockets.

The current status of commands support can be found in [STATUS.md](https://github.com/Peter2121/nmdaemon/blob/master/STATUS.md)

To build `nmdaemon`, two external libraries are needed:

- sockpp (https://github.com/fpagliughi/sockpp)
- asyncplusplus (https://github.com/Amanieu/asyncplusplus)

Static versions of these libraries must be installed.
Both libraries are present in FreeBSD ports tree (`net/libsockpp` and `devel/asyncplusplus`), static versions are built by default.

`nmdaemon` can be installed as `/usr/local/bin/nmdaemon` and started as FreeBSD service with rc.d script provided.

To build and install `nmdaemon`:

    git clone https://github.com/Peter2121/nmdaemon.git
    cd nmdaemon
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

The `nmdaemon` needs a configuration file, an example is provided and installed as `/usr/local/etc/nmdaemon.conf.sample`. To use this configuration file just copy it to `/usr/local/etc/nmdaemon.conf`.
The full name of the configuration file to use can be passed as a command-line argument:
`sudo /usr/local/bin/nmdaemon /home/user1/nmdaemon.conf`

If the configuration file is not provided in command line and `nmdaemon.conf` is not present in `/usr/local/etc` - some default configuration values will be used.

Some functional tests of `nmdaemon` using `nc` tool are provided in [tests](https://github.com/Peter2121/nmdaemon/tree/master/tests).
Attention, these tests can change network configuration of the host where they are launched (use with care).

The API is (almost) stable, the FreeBSD port will be created shortly after the release of 0.1 version.
