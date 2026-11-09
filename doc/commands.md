## Commands format

In general, all commands have the following format:

    { "SCOPE": "<scope>", "CMD": "<command>", "DATA": {<data_object>} }

The following scopes are supported in the current state:

SYSTEM
INTERFACE
ROUTE
WPA
WIFI

Every scope has his own set of commands. Every command needs his own data\_object.

## Objects

The following objects can be used in data\_object (as sub-objects or in arrays of objects):

**IPV4 ADDRESS** group defines IPv4 address with mask and (optionally) default gateway and mark ‘primary’.

    { "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>"[, "IPV4 GATEWAY": "<ip_address>"][, "PRIMARY": true|false] }

Examples:

    { "IPV4 ADDRESS": "192.168.212.216", "IPV4 GATEWAY": "192.168.212.1", "IPV4 SUBNET MASK": "255.255.255.0", "PRIMARY": true }

    { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0", "IPV4 GATEWAY": "192.168.211.2" }

    { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0" }


## Commands

**Run-time and boot-time configurations**

All commands from SYSTEM, INTERFACE and ROUTE  scopes concerns the current run-time configuration. The only command that changes the boot configuration is RCCONF\_WRITE.

The commands from WPA scope changes the run-time configuration of wpa\_supplicant,  but the modifications are not written automatically into the configuration file of wpa\_supplicant. The command WPA\_SAVE of WPA scope writes the current run-time configuration into the configuration file, so it will be applied on the next start of wpa\_supplicant.


**Scope SYSTEM commands**

    { "SCOPE": "SYSTEM", "CMD": "IF_LIST"[, "DATA": { "FILTER": { "MODE": "INCLUDE|EXCLUDE", "INTERFACE TYPE": [ "<type_1>", "<type_2>", ... ] } }] }

List all network interfaces, their IP addresses, status ("UP"/"RUNNING"/media), their "DRIVER NAME" (the original kernel-assigned name, e.g. "igb0", as reported by `ifconfig -D`; stays available after the interface has been renamed with IF_RENAME/IF_EDIT) and "DESCRIPTION" (if one was set); for BRIDGE interfaces, their current "MEMBERS" list; for LAGG interfaces, their current "MEMBERS" (ports) list, "LAGG PROTOCOL", and "MASTER" (the name of the port currently holding the MASTER flag, as shown by `ifconfig <lagg_name>`'s `laggport: ... flags=...<MASTER,...>` lines; omitted if no port currently has it). If the interface's current MAC address (see MAC_ADDR_SET) differs from its permanent/burned-in one, the permanent one is also reported as "HARDWARE ADDRESS" -- matching `ifconfig`'s own `ether <current>` / `hwaddr <permanent>` distinction; omitted when they match or the interface has no permanent address. Driver-reported hardware offload capabilities (currently "TXCSUM" and "TXCSUM6", see IF_EDIT) are reported as "ON"/"OFF"; a capability is omitted entirely when the interface's driver doesn't support it (more capabilities may be added to this list over time).

An optional "DATA"."FILTER" narrows the returned list. "MODE" is required and is either "INCLUDE" (only interfaces matching every given criterion are returned) or "EXCLUDE" (interfaces matching every given criterion are removed, everything else is returned). Currently the only supported criterion is "INTERFACE TYPE", an array of the same type strings used by IF_ADD ("TUN", "TAP", "EPAIR", "BRIDGE", "LAGG"); an interface that is none of these (e.g. a physical NIC or wlan0) never matches it. More criteria (e.g. "INTERFACE NAME", "STATUS", "MEDIA") may be added over time; when several are present in the same FILTER, an interface must satisfy all of them to match. An invalid or malformed FILTER returns `{ "RESULT": "ERROR" }`.

For example:

    { "SCOPE": "SYSTEM", "CMD": "IF_LIST", "DATA": { "FILTER": { "MODE": "INCLUDE", "INTERFACE TYPE": [ "TUN", "TAP" ] } } }
    { "SCOPE": "SYSTEM", "CMD": "IF_LIST", "DATA": { "FILTER": { "MODE": "EXCLUDE", "INTERFACE TYPE": [ "BRIDGE", "LAGG" ] } } }

---

    { "SCOPE": "SYSTEM", "CMD": "IF_ADD", "DATA" : { "INTERFACE NAME" : "<interface name>", "INTERFACE TYPE" : "<interface type>" } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "bridgeX", "INTERFACE TYPE" : "BRIDGE", "MEMBERS": [ "<if_1>", "<if_2>", ... ] } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "laggX", "INTERFACE TYPE" : "LAGG"[, "LAGG TYPE": "ETHERNET|INFINIBAND"][, "LAGG PROTOCOL": "FAILOVER|LACP|LOADBALANCE|ROUNDROBIN|BROADCAST|NONE"][, "RR_LIMIT": <stride>][, "MEMBERS": [ "<if_1>", "<if_2>", ... ]] } }


Add network interface. Currently supported the following interface types: TUN, TAP, BRIDGE, EPAIR, LAGG. For BRIDGE and LAGG types the members ("MEMBERS"/laggport) can be mentioned; the interface is automatically brought up (and its members/ports attached) once created. For EPAIR type the interface name must not include the trailing "a"/"b" side letter (e.g. "epair5"); the daemon creates both sides of the pair and returns the "a" side interface name.

For LAGG: "LAGG TYPE" selects the underlying link type (defaults to "ETHERNET" if omitted) and must be set at creation time. "LAGG PROTOCOL" selects the aggregation protocol (defaults to the kernel's own default, "FAILOVER", if omitted). "RR_LIMIT" sets the round-robin stride and is only accepted when "LAGG PROTOCOL" is "ROUNDROBIN". "MEMBERS" lists the ports (laggport) to attach, in the same way as for BRIDGE.

For example:

    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "tun5", "INTERFACE TYPE" : "TUN" } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "tap5", "INTERFACE TYPE" : "TAP" } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "epair5", "INTERFACE TYPE" : "EPAIR" } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "bridge5", "INTERFACE TYPE" : "BRIDGE", "MEMBERS": [ "re0", "em0" ] } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "lagg5", "INTERFACE TYPE" : "LAGG", "LAGG PROTOCOL": "LACP", "MEMBERS": [ "re0", "em0" ] } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_ADD", "DATA" : { "INTERFACE NAME" : "lagg6", "INTERFACE TYPE" : "LAGG", "LAGG PROTOCOL": "ROUNDROBIN", "RR_LIMIT": 16, "MEMBERS": [ "re0", "em0" ] } }

---

    { "SCOPE" : "SYSTEM", "CMD" : "IF_REMOVE", "DATA" : { "INTERFACE NAME" : "<interface name>" } }

Remove network interface. Currently supported the following interface types: TUN, TAP, BRIDGE, EPAIR. For EPAIR, destroying either side ("epair5a" or "epair5b") destroys both sides of the pair; the bare base name ("epair5") is also accepted and resolved to the "a" side.

For example:

    { "SCOPE" : "SYSTEM", "CMD" : "IF_REMOVE", "DATA" : { "INTERFACE NAME" : "bridge5" } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_REMOVE", "DATA" : { "INTERFACE NAME" : "epair5a" } }
    { "SCOPE" : "SYSTEM", "CMD" : "IF_REMOVE", "DATA" : { "INTERFACE NAME" : "epair5" } }

---

    { "SCOPE": "SYSTEM", "CMD": "JAIL_LIST" }

List all jails and their IP addresses

---

    { "SCOPE": "SYSTEM", "CMD": "RCCONF_READ" }

Read interfaces and routing information from rc.conf

---

    { "SCOPE" : "SYSTEM", "CMD" : "RCCONF_WRITE", "DATA": { "INTERFACES": [ { "ADDRESSES": [ <ipv4_address_object1>, <ipv4_address_object2>, ... ], "INTERFACE NAME": "<if_name>" }, ... ], "ROUTES": [ { "ROUTE NAME": "<rt_name>", "STATUS": "ENABLED|DISABLED", "ADDRESSES": <ipv4_address_object1> }, ...] } }

Write interfaces and/or routing information into rc.conf

For example:

    { "SCOPE": "SYSTEM", "CMD": "RCCONF_WRITE", "DATA": { "INTERFACES": [ { "ADDRESSES": [ { "IPV4 ADDRESS": "192.168.212.216", "IPV4 GATEWAY": "192.168.212.1", "IPV4 SUBNET MASK": "255.255.255.0", "PRIMARY": true }, { "IPV4 ADDRESS": "10.2.0.253", "IPV4 SUBNET MASK": "255.255.240.0" } ], "INTERFACE NAME": "em0" }, { "ADDRESSES": [ { "IPV4 ADDRESS": "WPA SYNCDHCP", "PRIMARY": true } ], "INTERFACE NAME": "wlan0" } ] } }

    { "SCOPE": "SYSTEM", "CMD": "RCCONF_WRITE", "DATA": { "INTERFACES": [ { "ADDRESSES": [ { "IPV4 ADDRESS": "192.168.212.215", "IPV4 GATEWAY": "192.168.212.1", "IPV4 SUBNET MASK": "255.255.255.0", "PRIMARY": true }, { "IPV4 ADDRESS": "192.168.212.102", "IPV4 SUBNET MASK": "255.255.255.0" }, { "IPV4 ADDRESS": "192.168.212.202", "IPV4 SUBNET MASK": "255.255.255.0" } ], "INTERFACE NAME": "em0" }, { "ADDRESSES": [ { "IPV4 ADDRESS": "10.255.0.252", "IPV4 SUBNET MASK": "255.255.255.240" }, { "IPV4 ADDRESS": "10.255.1.252", "IPV4 SUBNET MASK": "255.255.255.240" } ], "INTERFACE NAME": "em0.21" } ], "ROUTES":[ { "ADDRESSES": { "IPV4 ADDRESS": "192.168.253.0", "IPV4 GATEWAY": "192.168.212.250", "IPV4 SUBNET MASK": "255.255.255.128" },"ROUTE NAME": "net1", "STATUS": "ENABLED" }, { "ADDRESSES": { "IPV4 ADDRESS": "192.168.253.128", "IPV4 GATEWAY": "192.168.212.250", "IPV4 SUBNET MASK": "255.255.255.128" }, "ROUTE NAME": "net2", "STATUS": "ENABLED" } ] } }

---

    { "SCOPE": "SYSTEM", "CMD": "RESOLVCONF_READ" }

Read name resolution information from resolv.conf

---

    { "SCOPE": "SYSTEM", "CMD": "RESOLVCONF_WRITE", "DATA": { "DOMAIN": <domain_name>, "NAMESERVERS": [ <name_server_1>, <name_server_2>, ... ], "SEARCH DOMAINS": [ <search_domain_1>, <search_domain_2>, ... ] } }

Write name resolution information into resolv.conf

For example:

    {"SCOPE": "SYSTEM", "CMD": "RESOLVCONF_WRITE", "DATA": { "DOMAIN": "domain11.local", "NAMESERVERS": [ "1.1.1.1", "2.2.2.2" ], "SEARCH DOMAINS": [ "domain11.local", "domain22.local" ] } }

---

    { "SCOPE": "SYSTEM", "CMD": "IF_ENABLE", "DATA": { "INTERFACE NAME": "<if_name>" } }

    { "SCOPE": "SYSTEM", "CMD": "IF_DISABLE", "DATA": { "INTERFACE NAME": "<if_name>" } }

Enable/Disable network interface

For example:

    { "SCOPE": "SYSTEM", "CMD": "IF_ENABLE", "DATA": { "INTERFACE NAME": "em0" } }

    { "SCOPE": "SYSTEM", "CMD": "IF_DISABLE", "DATA": { "INTERFACE NAME": "em0" } }


**Scope INTERFACE commands**

    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "<if_name>"[, "NAME": "<new_if_name>"][, "MTU": <mtu_value>][, "MEMBERS": [ "<if_1>", "<if_2>", ... ]][, "DESCRIPTION": "<description>"][, "LAGG PROTOCOL": "FAILOVER|LACP|LOADBALANCE|ROUNDROBIN|BROADCAST|NONE"][, "RR_LIMIT": <stride>][, "LINK ADDRESS": "<mac_address>"][, "TXCSUM": "ON|OFF"][, "TXCSUM6": "ON|OFF"] } }

Edit one or more properties of an existing interface in a single command. Currently "NAME" (rename), "MTU", "MEMBERS" (for BRIDGE and LAGG interfaces), "DESCRIPTION", "LAGG PROTOCOL", "RR_LIMIT" (both LAGG-only), "LINK ADDRESS", "TXCSUM" and "TXCSUM6" are supported; more properties will be added over time.

If "NAME" is given, the interface is renamed before any other requested change is applied, so "MTU"/"MEMBERS"/"DESCRIPTION"/"LAGG PROTOCOL"/"RR_LIMIT"/"LINK ADDRESS"/"TXCSUM"/"TXCSUM6" in the same command already target the new name.

"MEMBERS" describes the desired final member list of the interface: for a BRIDGE, its member interfaces; for a LAGG, its ports (laggport). Members currently attached but missing from the list are removed, and members in the list not yet attached are added.

"DESCRIPTION" sets the interface description (equivalent to `ifconfig <if_name> description "<description>"`); an empty string clears it.

"LAGG PROTOCOL" changes the aggregation protocol of an existing LAGG interface; returns an error for any other interface type. "RR_LIMIT" sets the round-robin stride and is only accepted for a LAGG interface that is (or is being, in the same command) set to the "ROUNDROBIN" protocol.

"LINK ADDRESS" sets the interface's current MAC address, equivalent to MAC_ADDR_SET (`ifconfig <if_name> ether <mac_address>`). The hardware's permanent address is unaffected and, once they differ, is reported separately by IF_GET/IF_LIST as "HARDWARE ADDRESS".

"TXCSUM"/"TXCSUM6" enable or disable IPv4/IPv6 transmit checksum offloading, equivalent to `ifconfig <if_name> txcsum`/`-txcsum`/`txcsum6`/`-txcsum6`. Returns an error if the interface's driver doesn't support the requested capability.

For example:

    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "em0", "MTU": 1400 } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "bridge5", "MEMBERS": [ "re0", "vboxnet0" ] } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "vnet0", "NAME": "eth0" } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "em0", "DESCRIPTION": "uplink to ISP" } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "lagg5", "LAGG PROTOCOL": "LACP", "MEMBERS": [ "re0", "em0" ] } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "lagg5", "LAGG PROTOCOL": "ROUNDROBIN", "RR_LIMIT": 16 } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "em0", "LINK ADDRESS": "02:01:02:03:04:05" } }
    { "SCOPE": "INTERFACE", "CMD": "IF_EDIT", "DATA": { "INTERFACE NAME": "em0", "TXCSUM": "OFF", "TXCSUM6": "OFF" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IF_GET", "DATA": { "INTERFACE NAME": "<if_name>" } }

Return the same information as IF_LIST (IP addresses, status, media, "DRIVER NAME", "DESCRIPTION", "HARDWARE ADDRESS" when different from the current MAC, driver-supported capability status such as "TXCSUM"/"TXCSUM6", "MEMBERS" for BRIDGE interfaces, and "MEMBERS"/"LAGG PROTOCOL"/"MASTER" for LAGG interfaces), but for a single named interface. Returns `{ "RESULT": "NOTFOUND" }` if the interface does not exist.

For example:

    { "SCOPE": "INTERFACE", "CMD": "IF_GET", "DATA": { "INTERFACE NAME": "em0" } }
    { "SCOPE": "INTERFACE", "CMD": "IF_GET", "DATA": { "INTERFACE NAME": "bridge5" } }
    { "SCOPE": "INTERFACE", "CMD": "IF_GET", "DATA": { "INTERFACE NAME": "lagg5" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IF_RENAME", "DATA": { "INTERFACE NAME": "<if_name>", "NAME": "<new_if_name>" } }

Rename an existing interface, equivalent to `ifconfig <if_name> name <new_if_name>`.

For example:

    { "SCOPE": "INTERFACE", "CMD": "IF_RENAME", "DATA": { "INTERFACE NAME": "vnet0", "NAME": "eth0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IP_ADDR_SET", "DATA": { "INTERFACE NAME": "<if_name>", "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>" } }

Set primary IP address of the interface

For example:

    { "SCOPE": "INTERFACE", "CMD": "IP_ADDR_SET", "DATA": { "INTERFACE NAME": "em0", "IPV4 ADDRESS": "192.168.212.5", "IPV4 SUBNET MASK": "255.255.255.0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IP_ADDR_ADD", "DATA": { "INTERFACE NAME": "<if_name>", "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>" } }

Add IP alias to the interface

For example:

    { "SCOPE": "INTERFACE", "CMD": "IP_ADDR_ADD", "DATA": { "INTERFACE NAME": "em0", "IPV4 ADDRESS": "192.168.212.45", "IPV4 SUBNET MASK": "255.255.255.0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IP_ADDR_REMOVE", "DATA": { "INTERFACE NAME": "<if_name>", "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>" } }

Remove IP alias from the interface

For example:

    { "SCOPE": "INTERFACE", "CMD": "IP_ADDR_REMOVE", "DATA": { "INTERFACE NAME": "em0", "IPV4 ADDRESS": "192.168.212.45", "IPV4 SUBNET MASK": "255.255.255.0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IP4_DHCP_ENABLE", "DATA": { "INTERFACE NAME": "<if_name>" } }

    { "SCOPE": "INTERFACE", "CMD": "IP4_DHCP_DISABLE", "DATA": { "INTERFACE NAME": "<if_name>" } }

Enable/Disable automatic IP address assignment through DHCP protocol for the interface

For example:

    { "SCOPE": "INTERFACE", "CMD": "IP4_DHCP_ENABLE", "DATA": { "INTERFACE NAME": "em0" } }

    { "SCOPE": "INTERFACE", "CMD": "IP4_DHCP_DISABLE", "DATA": { "INTERFACE NAME": "em0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IP4_GET_DHCP_STATUS", "DATA": { "INTERFACE NAME": "<if_name>" } }

Check if automatic IP address assignment through DHCP protocol for the interface is enabled

For example:

    { "SCOPE": "INTERFACE", "CMD": "IP4_GET_DHCP\_STATUS", "DATA": { "INTERFACE NAME": "em0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "IP4_ADDR_GET", "DATA": { "INTERFACE NAME": "<if_name>" } }

Get (primary) IP address of the interface

*NOTE: Primary address can be swapped by system with an alias address on any modification of IP configuration, there is no documented method to fix it*

For example:

    { "SCOPE": "INTERFACE", "CMD": "IP4_ADDR_GET", "DATA": { "INTERFACE NAME": "em0" } }

---

    { "SCOPE": "INTERFACE", "CMD": "MTU_GET", "DATA": { "INTERFACE NAME" : "<if_name>" } }

    { "SCOPE": "INTERFACE", "CMD": "MTU_SET", "DATA": { "INTERFACE NAME" : "<if_name>", "MTU": <mtu_value> } }

Get/Set MTU value for the interface

For example:

    { "SCOPE": "INTERFACE", "CMD": "MTU_GET", "DATA": { "INTERFACE NAME": "em0" } }

    { "SCOPE": "INTERFACE", "CMD": "MTU_SET", "DATA": { "INTERFACE NAME": "em0", "MTU": 1400 } }

---

    { "SCOPE": "INTERFACE", "CMD": "MAC_ADDR_GET", "DATA": { "INTERFACE NAME" : "<if_name>" } }

    { "SCOPE": "INTERFACE", "CMD": "MAC_ADDR_SET", "DATA": { "INTERFACE NAME" : "<if_name>", "LINK ADDRESS": "<mac_address>" } }

Get/Set the current MAC (link-layer) address of the interface, equivalent to `ifconfig <if_name> ether <mac_address>`. This changes the interface's active address only; the hardware's permanent/burned-in address is unaffected and, once they differ, is reported separately as "HARDWARE ADDRESS" by IF_GET/IF_LIST.

For example:

    { "SCOPE": "INTERFACE", "CMD": "MAC_ADDR_GET", "DATA": { "INTERFACE NAME": "em0" } }

    { "SCOPE": "INTERFACE", "CMD": "MAC_ADDR_SET", "DATA": { "INTERFACE NAME": "em0", "LINK ADDRESS": "02:01:02:03:04:05" } }


**Scope ROUTE commands**

    { "SCOPE": "ROUTE", "CMD": "RT_DEF_GET" }

Get default IPv4 route

---

    { "SCOPE": "ROUTE", "CMD": "RT_DEF6_GET" }

Get default IPv6 route

---

    { "SCOPE": "ROUTE", "CMD": "RT_DEF_DEL" }

Delete default IPv4 route

---

    { "SCOPE": "ROUTE", "CMD": "RT_LIST" }

List all IPv4 routes

---

    { "SCOPE": "ROUTE", "CMD": "RT_LIST6" }

List all IPv6 routes

---

    { "SCOPE": "ROUTE", "CMD": "RT_DEF_SET", "DATA": { "IPV4 GATEWAY": "<ip_address>"[, "INTERFACE NAME": "<if_name>"] } }

Set default IPv4 route

If "INTERFACE NAME" is given, the route is forced to use that interface, equivalent to passing `-ifp <if_name>` to `route(8)`.

For example:

    { "SCOPE": "ROUTE", "CMD": "RT_DEF_SET", "DATA": { "IPV4 GATEWAY": "192.168.212.2" } }
    { "SCOPE": "ROUTE", "CMD": "RT_DEF_SET", "DATA": { "IPV4 GATEWAY": "192.168.212.2", "INTERFACE NAME": "em0" } }

---

    { "SCOPE": "ROUTE", "CMD": "RT_SET", "DATA": { "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>", "IPV4 GATEWAY": "<ip_address>"[, "INTERFACE NAME": "<if_name>"] } }

Add IPv4 static route

If "INTERFACE NAME" is given, the route is forced to use that interface, equivalent to passing `-ifp <if_name>` to `route(8)`.

For example:

    { "SCOPE": "ROUTE", "CMD": "RT_SET", "DATA": { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0", "IPV4 GATEWAY": "192.168.211.2" } }
    { "SCOPE": "ROUTE", "CMD": "RT_SET", "DATA": { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0", "IPV4 GATEWAY": "192.168.211.2", "INTERFACE NAME": "em0" } }

---

    { "SCOPE": "ROUTE", "CMD": "RT_GET", "DATA": { "IPV4 ADDRESS": "<ip_address>"[, "IPV4 SUBNET MASK": "<ip_address>"] } }

Get IPv4 route to the destination network/host

For example:

    { "SCOPE": "ROUTE", "CMD": "RT_GET", "DATA": { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0" } }

    { "SCOPE": "ROUTE", "CMD": "RT_GET", "DATA": { "IPV4 ADDRESS": "8.8.8.8" } }

---

    { "SCOPE": "ROUTE", "CMD": "RT_DEL", "DATA": { "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>", "IPV4 GATEWAY": "<ip_address>" } }

Delete IPv4 static route

For example:

    { "SCOPE": "ROUTE", "CMD": "RT_DEL", "DATA": { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0", "IPV4 GATEWAY": "192.168.211.2" } }


**Scope WPA commands**

The commands are sent to wpa\_supplicant daemon, it must be started, the communication socket(s) must be created at usual path

    { "SCOPE": "WPA", "CMD": "WPA_LIST_IF" }

List network interfaces available for wpa\_supplicant

---

    { "SCOPE": "WPA", "CMD": "WPA_LIST", "DATA": { "INTERFACE NAME": "<if_name>" } }

    { "SCOPE": "WPA", "CMD": "WPA_LIST", "DATA": { "INTERFACE NAME": "<if_name>", "DETAILS": false } }

List wireless networks, configured in wpa\_supplicant (only base parameters are included in the list: network id, ssid, bssid, flags)

For example:

    { "SCOPE": "WPA", "CMD": "WPA_LIST", "DATA": { "INTERFACE NAME": "wlan0", "DETAILS": false } }

---

    { "SCOPE": "WPA", "CMD": "WPA_LIST", "DATA": { "INTERFACE NAME": "<if_name>", "DETAILS": true } }

List wireless networks, configured in wpa\_supplicant (base parameters and additional parameters are included in the list : network id, ssid, bssid, flags, key\_mgmt, priority, proto)

*NOTE: This command can take several seconds to complete if there are many networks configured*

For example:

    { "SCOPE": "WPA", "CMD": "WPA_LIST", "DATA": { "INTERFACE NAME": "wlan0", "DETAILS": true } }

---

    { "SCOPE": "WPA", "CMD": "WPA_LIST_ASYNC", "SOCKET": "<client_socket_path>", "REQUEST": "<request_id>", "DATA": { "INTERFACE NAME": "<if_name>"[, "DETAILS": true] } }

List wireless networks, configured in wpa\_supplicant, asynchronously, without blocking the connection it was sent on

`<client_socket_path>` - path of a unix socket opened by the client, where it wants to receive the result of the operation. Must be a socket located directly in `/tmp` or `/var/tmp`, and must not be owned by root

`<request_id>` - arbitrary identifier chosen by the client, echoed back in the result so it can match it to this request. Must be 1 to 32 alphanumeric characters (`[A-Za-z0-9]`)

nmdaemon replies on the connection the command was sent on with `{ "RESULT": "SUCCESS" }` as soon as it has accepted the job and connected to `<client_socket_path>` (or `{ "RESULT": "ERROR" }` if `<request_id>` or `<client_socket_path>` is invalid or could not be connected to, or `{ "RESULT": "BUSY" }` if another operation is already running on this worker, in which case no further result follows). The actual listing then runs in the background exactly like WPA\_LIST; when it completes, nmdaemon connects to `<client_socket_path>` and sends `{ "RESULT": "SUCCESS", "REQUEST": "<request_id>" }` or `{ "RESULT": "ERROR", "REQUEST": "<request_id>" }`

*NOTE: this notification does not carry the list of networks itself – fetch it with WPA\_LIST\_RESULTS, passing the same `INTERFACE NAME` and `DETAILS` used here, within 30 seconds of completion*

For example:

    { "SCOPE": "WPA", "CMD": "WPA_LIST_ASYNC", "SOCKET": "/var/tmp/nmdcli11451", "REQUEST": "phaxawoh3kah", "DATA": { "INTERFACE NAME": "wlan0", "DETAILS": true } }

---

    { "SCOPE": "WPA", "CMD": "WPA_LIST_RESULTS", "DATA": { "INTERFACE NAME": "<if_name>"[, "DETAILS": true] } }

Get the result of the last successful WPA\_LIST\_ASYNC call for this `INTERFACE NAME` and `DETAILS` value

Only a WPA\_LIST\_ASYNC that completed with `"RESULT": "SUCCESS"` is kept for retrieval (a failed one is only reported through its own async notification); the result is kept available for 30 seconds after that, and only for the exact `INTERFACE NAME`/`DETAILS` it was requested with. Otherwise – including if no WPA\_LIST\_ASYNC ever completed successfully – this returns `{ "RESULT": "ERROR", "ERROR": "NO DATA" }` and discards whatever was cached

For example:

    { "SCOPE": "WPA", "CMD": "WPA_LIST_RESULTS", "DATA": { "INTERFACE NAME": "wlan0", "DETAILS": true } }

---

    { "SCOPE": "WPA", "CMD": "WPA_STATUS", "DATA": { "INTERFACE NAME": "<if_name>" } }

Get current status of the wireless interfaces

For example:

    { "SCOPE": "WPA", "CMD": "WPA_STATUS", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SCAN", "DATA": { "INTERFACE NAME": "<if_name>" } }

Start scan of wireless networks, return the result

*NOTE: if the scan request is failed – the results of previous scan is returned (like WPA\_SCAN\_RESULTS below)*

For example:

    { "SCOPE": "WPA", "CMD": "WPA_SCAN", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SCAN_ASYNC", "SOCKET": "<client_socket_path>", "REQUEST": "<request_id>", "DATA": { "INTERFACE NAME": "<if_name>" } }

Start scan of wireless networks asynchronously, without blocking the connection it was sent on

`<client_socket_path>` - path of a unix socket opened by the client, where it wants to receive the result of the operation. Must be a socket located directly in `/tmp` or `/var/tmp`, and must not be owned by root

`<request_id>` - arbitrary identifier chosen by the client, echoed back in the result so it can match it to this request. Must be 1 to 32 alphanumeric characters (`[A-Za-z0-9]`)

nmdaemon replies on the connection the command was sent on with `{ "RESULT": "SUCCESS" }` as soon as it has accepted the job and connected to `<client_socket_path>` (or `{ "RESULT": "ERROR" }` if `<request_id>` or `<client_socket_path>` is invalid or could not be connected to, or `{ "RESULT": "BUSY" }` if another operation is already running on this worker, in which case no further result follows). The actual scan then runs in the background exactly like WPA\_SCAN; when it completes, nmdaemon connects to `<client_socket_path>` and sends `{ "RESULT": "SUCCESS", "REQUEST": "<request_id>" }` or `{ "RESULT": "ERROR", "REQUEST": "<request_id>" }`

For example:

    { "SCOPE": "WPA", "CMD": "WPA_SCAN_ASYNC", "SOCKET": "/var/tmp/nmdcli11451", "REQUEST": "phaxawoh3kah", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SCAN_RESULTS", "DATA": { "INTERFACE NAME": "<if_name>" } }

Get the results of the last scan of wireless networks

For example:

    { "SCOPE" : "WPA", "CMD" : "WPA_SCAN\_RESULTS", "DATA" : { "INTERFACE NAME" : "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_ADD", "DATA": { "INTERFACE NAME": "<if_name>", "SSID": "<net_ssid>"[, "PSK": "<net_psk>"][, "PROFILE": "<security_profile>"] } }

Add new wireless network to wpa\_supplicant configuration

Parameters:

`<if_name>` - network interface name

`<net_ssid>` - SSID of the network

`<net_psk>` - security key of the network (for WPA, WPA2 and WEP networks)

*NOTE: WEP keys in Hex format are not (yet) supported*

`<security_profile>` - type of security of the network (currently supported types are WPA-PSK, WPA2-PSK, WEP, OPEN)

If the parameter `<security_profile>` is omitted – the type will be detected automatically 

For example:

    { "SCOPE": "WPA", "CMD": "WPA_ADD", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "rrr", "PSK": "rrrrrrrr" } }

    { "SCOPE": "WPA", "CMD": "WPA_ADD", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "testwpa", "PSK": "testwpaa", "PROFILE": "WPA-PSK" } }

    { "SCOPE": "WPA", "CMD": "WPA_ADD", "DATA": { "INTERFACE NAME" : "wlan0", "SSID": "testwpa2", "PSK": "testwpa2", "PROFILE": "WPA2-PSK" } }

    { "SCOPE": "WPA", "CMD": "WPA_ADD", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "testopen", "PROFILE": "OPEN" } }

    { "SCOPE": "WPA", "CMD": "WPA_ADD", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "testwep", "PSK": "testwepp", "PROFILE": "WEP" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SETPSK", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id>, "PSK": "<net_psk>" } }

Set security key for protected network (WPA-PSK, WPA2-PSK, WEP)

*NOTE: WEP keys in Hex format are not (yet) supported*

Parameters:

`<if_name>` - network interface name

`<net_id>` - ID of the network (can be found using WPA\_LIST command)

`<net_psk>` - security key of the network 

For example:

    { "SCOPE": "WPA", "CMD": "WPA_SETPSK", "DATA": { "INTERFACE NAME": "wlan0", "NETID": 28, "PSK": "rrrrrrrr" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SET_BSSID", "DATA": { "INTERFACE NAME": "<if_name>", "NETID" : <net_id>, "BSSID" : "<bss_id>" } }

Set BSSID parameter for wireless network

For example:

    { "SCOPE": "WPA", "CMD": "WPA_SET_BSSID", "DATA": { "INTERFACE NAME": "wlan0", "NETID" : 13, "BSSID" : "fe:cb:ac:8b:26:b5" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_ENABLE", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id> } }

    { "SCOPE": "WPA", "CMD": "WPA_DISABLE", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id> } }

Enable/disable wireless network

For example:

    { "SCOPE": "WPA", "CMD": "WPA_ENABLE", "DATA": { "INTERFACE NAME": "wlan0", "NETID": 13 } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SETPARAM", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id>, "PARAM\_NAME": "<param_name>", "PARAM_VALUE": "<param_value>", "PARAM_QUOTES" : true|false } }

Set wireless network parameter

Any of parameters read with WPA\_LIST command can be set

For string parameters PARAM\_QUOTES must be ‘true’, for integer parameters it must be ‘false’

For example:

    { "SCOPE": "WPA", "CMD": "WPA_SETPARAM", "DATA": { "INTERFACE NAME": "wlan0", "NETID": 37, "PARAM_NAME": "key_mgmt", "PARAM_VALUE": "NONE", "PARAM_QUOTES" : false } }

---

    { "SCOPE": "WPA", "CMD": "WPA_REMOVE", "DATA": { "INTERFACE NAME": "<if_name>", "SSID": "<net_ssid>" } }

Remove wireless network from the configuration (by SSID)

For example:

    { "SCOPE": "WPA", "CMD": "WPA_REMOVE", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "rrr-rrr" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_REMOVE", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id> } }

Remove wireless network from the configuration (by network ID)

For example:

    { "SCOPE": "WPA", "CMD": "WPA_REMOVE", "DATA": { "INTERFACE NAME": "wlan0", "NETID": 30 } }

---

    { "SCOPE": "WPA", "CMD": "WPA_CONNECT", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "rrr-rrr" } }

Try to connect to the selected network (by SSID)

The priority of currently connected network will be decreased, the priority of the selected network will be increased to put it on top of the configured networks

For example:

    { "SCOPE": "WPA", "CMD": "WPA_CONNECT", "DATA": { "INTERFACE NAME": "wlan0", "SSID": "rrr-rrr" } }

---

    { "SCOPE" : "WPA", "CMD": "WPA_CONNECT", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id> } }

Try to connect to the selected network (by network ID)

The priority of currently connected network will be decreased, the priority of the selected network will be increased to put it on top of the configured networks

For example:

    { "SCOPE" : "WPA", "CMD": "WPA_CONNECT", "DATA": { "INTERFACE NAME": "wlan0", "NETID": 28 } }

---

    { "SCOPE": "WPA", "CMD": "WPA_CONNECT_ASYNC", "SOCKET": "<client_socket_path>", "REQUEST": "<request_id>", "DATA": { "INTERFACE NAME": "<if_name>", "SSID": "<net_ssid>" } }

    { "SCOPE": "WPA", "CMD": "WPA_CONNECT_ASYNC", "SOCKET": "<client_socket_path>", "REQUEST": "<request_id>", "DATA": { "INTERFACE NAME": "<if_name>", "NETID": <net_id> } }

Try to connect to the selected network (by SSID, BSSID or network ID) asynchronously, without blocking the connection it was sent on

`<client_socket_path>` - path of a unix socket opened by the client, where it wants to receive the result of the operation. Must be a socket located directly in `/tmp` or `/var/tmp`, and must not be owned by root

`<request_id>` - arbitrary identifier chosen by the client, echoed back in the result so it can match it to this request. Must be 1 to 32 alphanumeric characters (`[A-Za-z0-9]`)

nmdaemon replies on the connection the command was sent on with `{ "RESULT": "SUCCESS" }` as soon as it has accepted the job and connected to `<client_socket_path>` (or `{ "RESULT": "ERROR" }` if `<request_id>` or `<client_socket_path>` is invalid or could not be connected to, or `{ "RESULT": "BUSY" }` if another operation is already running on this worker, in which case no further result follows). The actual connection attempt then runs in the background exactly like WPA\_CONNECT; when it completes, nmdaemon connects to `<client_socket_path>` and sends `{ "RESULT": "SUCCESS", "REQUEST": "<request_id>" }` or `{ "RESULT": "ERROR", "REQUEST": "<request_id>" }`

For example:

    { "SCOPE": "WPA", "CMD": "WPA_CONNECT_ASYNC", "SOCKET": "/var/tmp/nmdcli11451", "REQUEST": "phaxawoh3kah", "DATA": { "INTERFACE NAME": "wlan0", "NETID": 28 } }

---

    { "SCOPE": "WPA", "CMD": "WPA_DISCONNECT", "DATA": { "INTERFACE NAME": "<if_name>" } }

Disconnect currently connected network

For example:

    { "SCOPE": "WPA", "CMD": "WPA_DISCONNECT", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_REASSOC", "DATA": { "INTERFACE NAME": "<if_name>" } }

Re-associate (reconnect) wireless network

For example:

    { "SCOPE": "WPA", "CMD": "WPA_REASSOC", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_RESET", "DATA": { "INTERFACE NAME": "<if_name>" } }

Reset wpa\_supplicant status

For example:

    { "SCOPE": "WPA", "CMD": "WPA_RESET", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WPA", "CMD": "WPA_SAVE", "DATA": { "INTERFACE NAME": "<if_name>" } }

Save current wpa\_supplicant configuration into it's configuration file

For example:

    { "SCOPE": "WPA", "CMD": "WPA_SAVE", "DATA": { "INTERFACE NAME": "wlan0" } }


**Scope WIFI commands**

The commands are sent to kernel using lib80211

    { "SCOPE": "WIFI", "CMD": "WIFI_SCAN", "DATA": { "INTERFACE NAME": "<if_name>" } }

Start scan of wireless networks, return the result

*NOTE: if the scan request is failed – the results of previous scan is returned (like WPA\_SCAN\_RESULTS below)*

For example:

    { "SCOPE": "WIFI", "CMD": "WIFI_SCAN", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WIFI", "CMD": "WIFI_SCAN_RESULTS", "DATA": { "INTERFACE NAME": "<if_name>" } }

Get the results of the last scan of wireless networks

For example:

    { "SCOPE": "WIFI", "CMD": "WIFI_SCAN_RESULTS", "DATA": { "INTERFACE NAME": "wlan0" } }

---

    { "SCOPE": "WIFI", "CMD": "WIFI_STATUS", "DATA": { "INTERFACE NAME": "<if_name>" } }

Get current status of the wireless interface

For example:

    { "SCOPE": "WIFI", "CMD": "WIFI_STATUS", "DATA": { "INTERFACE NAME": "wlan0" } }

