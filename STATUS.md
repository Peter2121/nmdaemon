| SCOPE        | CMD              | SYSTEM          | IPv4             | IPv6             |
| -------------| -----------------| ----------------| -----------------|------------------|
| SYSTEM       | IF_ADD           | Not Implemented | *Not Applicable* | *Not Applicable* |
| SYSTEM       | IF_REMOVE        | Not Implemented | *Not Applicable* | *Not Applicable* |
| SYSTEM       | IF_ENABLE        | Works           | *Not Applicable* | *Not Applicable* |
| SYSTEM       | IF_DISABLE       | Works           | *Not Applicable* | *Not Applicable* |
| SYSTEM       | IF_LIST          | Works           | Works            | Works            |
| SYSTEM       | RCCONF_READ      | Works           | Works            | Data Ignored     |
| SYSTEM       | RCCONF_WRITE     | Works           | Works            | Data Ignored     |
| INTERFACE    | IP\_ADDR_SET     | Works           | Works            | Not Implemented  |
| INTERFACE    | IP4\_ADDR_GET    | Works           | Works            | *Not Applicable* |
| INTERFACE    | IP6\_ADDR_GET    | Not Implemented | *Not Applicable* | Not Implemented  |
| INTERFACE    | IP4\_DHCP_ENABLE | Works           | Works            | *Not Applicable* |
| INTERFACE    | IP6\_DHCP_ENABLE | Not Implemented | *Not Applicable* | Not Implemented  |
| INTERFACE    | IP\_ADDR_ADD     | Works           | Works            | Not Implemented  |
| INTERFACE    | IP\_ADDR_REMOVE  | Works           | Works            | Not Implemented  |
| INTERFACE    | MTU_GET          | Works           | Works            | Works            |
| INTERFACE    | MTU_SET          | Works           | Works            | Works            |
| INTERFACE    | MAC\_ADDR_GET    | Not Implemented | *Not Applicable* | *Not Applicable* |
| INTERFACE    | MAC\_ADDR_SET    | Not Implemented | *Not Applicable* | *Not Applicable* |
| ROUTE        | RT_GET           | Works           | Works            | Works            |
| ROUTE        | RT\_DEF_GET      | Works           | Works            | *Not Applicable* |
| ROUTE        | RT\_DEF6_GET     | Works           | *Not Applicable* | Untested         |
| ROUTE        | RT_SET           | Works           | Works            | Untested         |
| ROUTE        | RT\_DEF_SET      | Works           | Works            | Untested         |
| ROUTE        | RT_DEL           | Works           | Works            | Untested         |
| ROUTE        | RT\_DEF_DEL      | Works           | Works            | *Not Applicable* |
| ROUTE        | RT_LIST          | Works           | Works            | *Not Applicable* |
| ROUTE        | RT_LIST6         | Works           | *Not Applicable* | Works            |
| WPA          | WPA_LIST_IF      | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_LIST         | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_SCAN         | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_STATUS       | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_SETPSK       | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_CONNECT      | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_DISCONNECT   | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_REASSOC      | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_ADD          | Works           | *Not Applicable* | *Not Applicable* |
| WPA          | WPA_REMOVE       | Works           | *Not Applicable* | *Not Applicable* |
| WIFI         | WIFI\_SCAN_RESULTS| Works           | *Not Applicable* | *Not Applicable* |
| WIFI         | WIFI_SCAN        | Works           | *Not Applicable* | *Not Applicable* |
| WIFI         | WIFI_STATUS      | Works           | *Not Applicable* | *Not Applicable* |
