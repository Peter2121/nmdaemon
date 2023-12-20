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

    { "SCOPE": "SYSTEM", "CMD": "IF_LIST" }

List all network interfaces and their IP addresses

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

    { "SCOPE": "SYSTEM", "CMD": "IF_ENABLE", "DATA": { "INTERFACE NAME": "<if_name>" } }

    { "SCOPE": "SYSTEM", "CMD": "IF_DISABLE", "DATA": { "INTERFACE NAME": "<if_name>" } }

Enable/Disable network interface

For example:

    { "SCOPE": "SYSTEM", "CMD": "IF_ENABLE", "DATA": { "INTERFACE NAME": "em0" } }

    { "SCOPE": "SYSTEM", "CMD": "IF_DISABLE", "DATA": { "INTERFACE NAME": "em0" } }


**Scope INTERFACE commands**

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

    { "SCOPE": "ROUTE", "CMD": "RT_DEF_SET", "DATA": { "IPV4 GATEWAY": "<ip_address>" } }

Set default IPv4 route

For example:

    { "SCOPE": "ROUTE", "CMD": "RT_DEF_SET", "DATA": { "IPV4 GATEWAY": "192.168.212.2" } }

---

    { "SCOPE": "ROUTE", "CMD": "RT_SET", "DATA": { "IPV4 ADDRESS": "<ip_address>", "IPV4 SUBNET MASK": "<ip_address>", "IPV4 GATEWAY": "<ip_address>" } }

Add IPv4 static route

For example:

    { "SCOPE": "ROUTE", "CMD": "RT_SET", "DATA": { "IPV4 ADDRESS": "99.99.99.0", "IPV4 SUBNET MASK": "255.255.255.0", "IPV4 GATEWAY": "192.168.211.2" } }

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

