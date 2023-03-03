#include "tool.h"
#include "addressgroup.h"
#include "mediadesc.h"

std::shared_ptr<AddressGroup> Tool::getAddrFromJson(json cmd)
{
    json cmd_json_data = {};
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    std::string str_ifgw = "";
    std::string str_ifbcast = "";
    short ip_family = 0;
    std::shared_ptr<AddressBase> sp_ifaddr = nullptr;
    std::shared_ptr<AddressBase> sp_ifmask = nullptr;
    std::shared_ptr<AddressBase> sp_ifdata = nullptr;
    AddressGroupType ip_type;
    std::shared_ptr<AddressGroup> sp_addr = nullptr;

    if(!cmd.contains(JSON_PARAM_DATA))
    {
        LOG_S(ERROR) << "getAddrFromJson - JSON does not contain data section";
        return sp_addr;
    }

    cmd_json_data = cmd[JSON_PARAM_DATA];

    if( cmd_json_data.contains(JSON_PARAM_IPV4_ADDR) ||
        cmd_json_data.contains(JSON_PARAM_IPV4_MASK) ||
        cmd_json_data.contains(JSON_PARAM_IPV4_BCAST) ||
        cmd_json_data.contains(JSON_PARAM_IPV4_GW) )
    {
        ip_family = AF_INET;
    }
    else if( cmd_json_data.contains(JSON_PARAM_IPV6_ADDR) ||
             cmd_json_data.contains(JSON_PARAM_IPV6_MASK) ||
             cmd_json_data.contains(JSON_PARAM_IPV6_BCAST) ||
             cmd_json_data.contains(JSON_PARAM_IPV6_GW) )
    {
        ip_family = AF_INET6;
    }
    else
    {
        LOG_S(ERROR) << "getAddrFromJson - cannot determine IP address family";
        return sp_addr;
    }

    switch(ip_family)
    {
        case AF_INET:
            if(cmd_json_data.contains(JSON_PARAM_IPV4_ADDR))
                str_ifaddr = cmd_json_data[JSON_PARAM_IPV4_ADDR];
            if(cmd_json_data.contains(JSON_PARAM_IPV4_MASK))
                str_ifmask = cmd_json_data[JSON_PARAM_IPV4_MASK];
            if(cmd_json_data.contains(JSON_PARAM_IPV6_BCAST))
            {
                str_ifbcast = cmd_json_data[JSON_PARAM_IPV6_BCAST];
                ip_type = AddressGroupType::BCAST;
            }
            else if(cmd_json_data.contains(JSON_PARAM_IPV4_GW))
            {
                str_ifgw = cmd_json_data[JSON_PARAM_IPV4_GW];
                ip_type = AddressGroupType::PPP;
            }
            break;
        case AF_INET6:
            if(cmd_json_data.contains(JSON_PARAM_IPV6_ADDR))
                str_ifaddr = cmd_json_data[JSON_PARAM_IPV6_ADDR];
            if(cmd_json_data.contains(JSON_PARAM_IPV6_MASK))
                str_ifmask = cmd_json_data[JSON_PARAM_IPV6_MASK];
            if(cmd_json_data.contains(JSON_PARAM_IPV6_BCAST))
            {
                str_ifbcast = cmd_json_data[JSON_PARAM_IPV6_BCAST];
                ip_type = AddressGroupType::BCAST;
            }
            else if(cmd_json_data.contains(JSON_PARAM_IPV6_GW))
            {
                str_ifgw = cmd_json_data[JSON_PARAM_IPV6_GW];
                ip_type = AddressGroupType::PPP;
            }
            break;
    }

    if( str_ifaddr.empty() && str_ifmask.empty() && str_ifgw.empty() && str_ifbcast.empty() )
    {
        LOG_S(ERROR) << "getAddrFromJson - cannot get IP address from JSON";
        return sp_addr;
    }

    switch(ip_family)
    {
        case AF_INET:
            try
            {
                if(!str_ifaddr.empty())
                    sp_ifaddr = std::make_shared<AddressIp4>(str_ifaddr);
                if(!str_ifmask.empty())
                    sp_ifmask = std::make_shared<AddressIp4>(str_ifmask);
                if( ip_type==AddressGroupType::BCAST && !str_ifbcast.empty() )
                    sp_ifdata=std::make_shared<AddressIp4>(str_ifbcast);
                else if( ((ip_type==AddressGroupType::PPP)||(ip_type==AddressGroupType::ROUTE)) && (!str_ifgw.empty()) )
                    sp_ifdata=std::make_shared<AddressIp4>(str_ifgw);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "Cannot create ip4 address from JSON parameters";
                return sp_addr;
            }
            break;
        case AF_INET6:
            try
            {
                if(!str_ifaddr.empty())
                    sp_ifaddr = std::make_shared<AddressIp6>(str_ifaddr);
                if(!str_ifmask.empty())
                    sp_ifmask = std::make_shared<AddressIp6>(str_ifmask);
                if( ip_type==AddressGroupType::BCAST && !str_ifbcast.empty() )
                    sp_ifdata=std::make_shared<AddressIp6>(str_ifbcast);
                else if( ((ip_type==AddressGroupType::PPP)||(ip_type==AddressGroupType::ROUTE)) && (!str_ifgw.empty()) )
                    sp_ifdata=std::make_shared<AddressIp6>(str_ifgw);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "Cannot create ip6 address from JSON parameters";
                return sp_addr;
            }
            break;
    }

    try {
        sp_addr = std::make_shared<AddressGroup>(sp_ifaddr, sp_ifmask, sp_ifdata, ip_type, false);
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot create if_addr from JSON parameters";
        return nullptr;
    }
    return sp_addr;
}

int Tool::getIfFlags(std::string ifname)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    sockpp::socket sock = sockpp::socket::create(AF_LOCAL, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot get interface " << ifname << " flags";
        sock.close();
        return 0;
    }
    sock.close();
    return ((ifr.ifr_flags & 0xffff) | (ifr.ifr_flagshigh << 16));
}

bool Tool::setIfFlags(std::string ifname, int setflags)
{
    sockpp::socket sock = sockpp::socket::create(AF_LOCAL, SOCK_DGRAM);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_flags = setflags & 0xffff;
    ifr.ifr_flagshigh = setflags >> 16;
    if (ioctl(sock.handle(), SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot set interface " << ifname << " flags";
        sock.close();
        return false;
    }
    sock.close();
    return true;
}

std::string Tool::getIfPrimaryAddr4(std::string ifname)
{
    struct ifreq ifr;
    std::string str_addr4;
    struct sockaddr_in *sa_in;
    char address[INET_ADDRSTRLEN];

    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);

    if (ioctl(sock.handle(), SIOCGIFADDR, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot get primary IPv4 address for interface: " << ifname;
        sock.close();
        return str_addr4;
    }
    sock.close();

    sa_in = (struct sockaddr_in *) &ifr.ifr_addr;
    if(sa_in->sin_addr.s_addr!=0) {
        inet_ntop( AF_INET, &(sa_in->sin_addr), address, sizeof(address) );
        str_addr4 = std::string(address);
    }

    return str_addr4;
}

bool Tool::isValidGw4(uint32_t addr, uint32_t mask, uint32_t gw)
{
    if( (addr==0) || (mask==0) || (gw==0) )
        return false;

    uint32_t result1 = (~mask) & addr;
    if(result1==0)   // No valuable bits in addr
        return false;
    uint32_t result2 = (~mask) & gw;
    if(result2==0)   // No valuable bits in gw
        return false;
    if( (result1^result2) == 0 ) // Overlapped addr and gw
        return false;
    if( ((~mask)^result2) ==0 )  // Gw is bcast address
        return false;

    result1 = mask & addr;
    result2 = mask & gw;
    if( (result1^result2) != 0 ) // addr and gw in different networks
        return false;

    return true;
}

bool Tool::isValidBcast4(uint32_t addr, uint32_t mask, uint32_t bcast)
{
    if( (addr==0) || (mask==0) || (bcast==0) )
        return false;

    if( ((~mask)&addr) == 0 )    // No valuable bits in addr
        return false;

    uint32_t result1 = (~mask) & bcast;
    if( ((~mask)^result1) != 0 )   // Overlapped mask and bcast
        return false;

    result1 = mask & addr;
    uint32_t result2 = mask & bcast;
    if( (result1^result2) != 0 ) // addr and bcast in different networks
        return false;

    return true;
}

std::vector<std::tuple<int, std::string, std::string>> Tool::getActiveProcesses()
{
    char errbuf[_POSIX2_LINE_MAX];
    int nentries = 0;
    char **args;
    char **ptr;
    char *buf = nullptr;
    size_t size = 0, pos = 0, len = 0, curlen=0; ;
    std::vector<std::tuple<int, std::string, std::string>> vect_procs;

    kvm_t *kernel = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
    if(kernel == nullptr)
    {
        LOG_S(ERROR) << "Error in getActiveProcesses: kvm_openfiles failed";
        LOG_S(ERROR) << errbuf;
        return vect_procs;
    }
    struct kinfo_proc *kinfo = kvm_getprocs(kernel, KERN_PROC_PROC, 0, &nentries);
    if(kinfo == nullptr)
    {
        LOG_S(ERROR) << "Error in getActiveProcesses: kvm_getprocs returns NULL";
        kvm_close(kernel);
        return vect_procs;
    }

    for (int i = 0; i < nentries; ++i)
    {
// Trying to get arguments of the process
        args = kvm_getargv(kernel, &kinfo[i], 0);
        if( (args != NULL) && (*args != NULL) && (**args != '\0') )
        {
            for (ptr = args; *ptr; ptr++)
            {
                size += strlen(*ptr)+1;
            }
            size += 2;
            buf = new char[size];
            pos = 0;
            for (ptr = args; *ptr; ptr++)
            {
                len = strlen(*ptr)+1;
                curlen = strlen(buf);
                if(curlen>0)
                {
                    buf[curlen] = ' ';
                    buf[curlen+1] = 0;
                }
                memcpy(buf+pos, *ptr, len);
                pos += len;
            }
        }
// Put process info into return vector
        if(buf != nullptr)
        {
            vect_procs.push_back(std::make_tuple(kinfo[i].ki_pid, kinfo[i].ki_comm, buf));
            delete[](buf);
            buf = nullptr;
        }
        else
        {
            vect_procs.push_back(std::make_tuple(kinfo[i].ki_pid, kinfo[i].ki_comm, ""));
        }
    }

    kvm_close(kernel);
    return vect_procs;
}

bool Tool::isDHCPEnabled(std::string ifname)
{
    std::vector<std::tuple<int, std::string, std::string>> procs = getActiveProcesses();
    bool found=false;
    for(auto &p : procs)
    {
        if(get<1>(p) == DHCP_CLIENT_PROCESS)
        {
            if(get<2>(p).find(ifname) != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    return found;
}

int Tool::getDHCPClientPid(std::string ifname)
{
    std::vector<std::tuple<int, std::string, std::string>> procs = getActiveProcesses();
    int pid = 0;
    for(auto &p : procs)
    {
        if(get<1>(p) == DHCP_CLIENT_PROCESS)
        {
            if(get<2>(p).find(ifname) != std::string::npos)
            {
                pid = get<0>(p);
                break;
            }
        }
    }
    return pid;
}

bool Tool::termDHCPClient(std::string ifname, short signal) // signal = SIGTERM (default), SIGKILL
{
    int pid = getDHCPClientPid(ifname);
    if(pid>0)
    {
        LOG_S(INFO) << "Sending signal " << signal << " to pid " << pid;
        if(kill(pid, signal) != 0)
        {
            LOG_S(ERROR) << "Error in termDHCPClient: cannot send signal " << signal << " to process " << pid;
            LOG_S(ERROR) << strerror(errno);
            return false;
        }
    }
    else
    {
        LOG_S(ERROR) << "Error in termDHCPClient: cannot find DHCP client running for interface " << ifname;
        return false;
    }
    return true;
}
bool Tool::isMediaStatusSupported(std::string ifname)
{
    for(auto prefix : MEDIA_BL_PREFIXES)
    {
        if(ifname.starts_with(prefix))
            return false;
    }
    return true;
}

std::unique_ptr<struct ifmediareq> Tool::getMediaState(std::string ifname)
{
    bool xmedia = true;
    std::unique_ptr<struct ifmediareq> ifmr = std::make_unique<struct ifmediareq>();
    std::unique_ptr<int[]> media_list;

    if(!isMediaStatusSupported(ifname))
        return ifmr;

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);

    memset(ifmr.get(), 0, sizeof(struct ifmediareq));
    strlcpy(ifmr.get()->ifm_name, ifname.c_str(), sizeof(ifmr.get()->ifm_name));

    if (ioctl(sock.handle(), SIOCGIFXMEDIA, (caddr_t)ifmr.get()) < 0)
        xmedia = false;
    if (!xmedia && ioctl(sock.handle(), SIOCGIFMEDIA, (caddr_t)ifmr.get()) < 0)
    {
        LOG_S(WARNING) << "Warning: Interface " << ifname << " doesn't support SIOCGIF(X)MEDIA";
        sock.close();
        return ifmr;
    }

    if (ifmr->ifm_count != 0)
    {
        media_list = std::make_unique<int[]>(ifmr->ifm_count);
        ifmr->ifm_ulist = &media_list[0];

        if (xmedia)
        {
            if (ioctl(sock.handle(), SIOCGIFXMEDIA, (caddr_t)ifmr.get()) < 0)
            {
                LOG_S(ERROR) << "Error in getMediaState: SIOCGIFXMEDIA ioctl failed for interface " << ifname;
            }
        }
        else
        {
            if (ioctl(sock.handle(), SIOCGIFMEDIA, (caddr_t)ifmr.get()) < 0)
            {
                LOG_S(ERROR) << "Error in getMediaState: SIOCGIFMEDIA ioctl failed for interface " << ifname;
            }
        }
    }
    sock.close();
    return ifmr;
}

MediaStatus Tool::getMediaStatus(std::string ifname)
{
    MediaStatus status = MediaStatus::UNKNOWN;
    std::unique_ptr<struct ifmediareq> ifmr = Tool::getMediaState(ifname);
    if(ifmr==nullptr)
        return status;

    if (ifmr->ifm_status & IFM_AVALID)
    {
        switch (IFM_TYPE(ifmr->ifm_active))
        {
            case IFM_ETHER:
            case IFM_ATM:
                if (ifmr->ifm_status & IFM_ACTIVE)
                    status = MediaStatus::ACTIVE;
                else
                    status = MediaStatus::NO_CARRIER;
                break;
            case IFM_IEEE80211:
                if (ifmr->ifm_status & IFM_ACTIVE)
                {
                    /* NB: only sta mode associates */
                    if (IFM_OPMODE(ifmr->ifm_active) == IFM_IEEE80211_STA)
                        status = MediaStatus::ASSOCIATED;
                    else
                        status = MediaStatus::RUNNING;
                }
                else
                    status = MediaStatus::NO_CARRIER;
                break;
        }
    }
    return status;
}

std::string Tool::getMediaDesc(std::string ifname)
{
    std::string str_desc;
    std::unique_ptr<struct ifmediareq> ifmr = Tool::getMediaState(ifname);
    if(ifmr==nullptr)
        return str_desc;

    str_desc += getDescWord(ifmr->ifm_current, 1);

    if (ifmr->ifm_active != ifmr->ifm_current)
    {
        str_desc += " (";
        str_desc += getDescWord(ifmr->ifm_active, 0);
        str_desc += ")";
    }

    return str_desc;
}

// ifconfig code is used here
std::string Tool::getDescWord(int ifmw, int print_toptype)
{
    std::string str_desc_word;
    struct ifmedia_description *desc;
    struct ifmedia_description *desc1;
    struct ifmedia_type_to_subtype *ttos;
    int seen_option = 0;
    bool found = false;

    /* Find the top-level interface type. */
//    desc = get_toptype_desc(ifmw);
    for (desc = ifm_type_descriptions; desc->ifmt_string != NULL; desc++)
    {
        if (IFM_TYPE(ifmw) == desc->ifmt_word)
            break;
    }

//    ttos = get_toptype_ttos(ifmw);
    for (desc1 = ifm_type_descriptions, ttos = ifmedia_types_to_subtypes;
        desc1->ifmt_string != NULL; desc1++, ttos++)
    {
        if (IFM_TYPE(ifmw) == desc1->ifmt_word)
            break;
    }

    if (desc->ifmt_string == NULL)
    {
        return str_desc_word;
    }
    else if (print_toptype)
    {
        str_desc_word += std::string(desc->ifmt_string);
    }
    /* Find subtype. */
//    desc = get_subtype_desc(ifmw, ttos);
    for (int i = 0; ttos->subtypes[i].desc != NULL; i++)
    {
        if (ttos->subtypes[i].alias>0)
            continue;
        desc = nullptr;
        found = false;
        for (desc = ttos->subtypes[i].desc; desc->ifmt_string != NULL; desc++)
        {
            if (IFM_SUBTYPE(ifmw) == desc->ifmt_word)
            {
                found = true;
                break;
            }
        }
        if(found)
            break;
    }

    if (desc == nullptr)
    {
        return str_desc_word;
    }

    if (print_toptype)
        str_desc_word += " ";

    if(desc->ifmt_string != NULL)
        str_desc_word += std::string(desc->ifmt_string);

    if (print_toptype)
    {
//        desc = get_mode_desc(ifmw, ttos);
        for (int i = 0; ttos->modes[i].desc != NULL; i++)
        {
            if (ttos->modes[i].alias)
                continue;
            desc = nullptr;
            found = false;
            for (desc = ttos->modes[i].desc; desc->ifmt_string != NULL; desc++)
            {
                if (IFM_MODE(ifmw) == desc->ifmt_word)
                {
                    found = true;
                    break;
                }
            }
            if(found)
                break;
        }
        if (desc != nullptr && desc->ifmt_string != NULL && strcasecmp("autoselect", desc->ifmt_string))
            str_desc_word += " mode " + std::string(desc->ifmt_string);
    }

    /* Find options. */
    for (int i = 0; ttos->options[i].desc != NULL; i++)
    {
        if (ttos->options[i].alias)
            continue;
        desc = nullptr;
        for (desc = ttos->options[i].desc; desc->ifmt_string != NULL; desc++)
        {
            if (ifmw & desc->ifmt_word)
            {
                if (seen_option == 0)
                    str_desc_word += " <";
                if(seen_option++ > 0)
                    str_desc_word += ",";
                str_desc_word += std::string(desc->ifmt_string);
//                printf("%s%s", seen_option++ ? "," : "", desc->ifmt_string);
            }
        }
    }
    if(seen_option > 0)
        str_desc_word += ">";

    if (print_toptype && IFM_INST(ifmw) != 0)
    {
        str_desc_word += " instance ";
        str_desc_word += std::to_string(IFM_INST(ifmw));
    }
    return str_desc_word;
}

std::vector<JailParam> Tool::getJails()
{
    int jflags=0;
    std::vector<JailParam> vect_jails;

    JailParam::InitRequestParams();
    while(true)
    {
        std::unique_ptr<JailParam> pjail = std::make_unique<JailParam>();
        JailParam::Lastjid = pjail->GetJailParams(jflags);
        if(JailParam::Lastjid<0)
            break;
        vect_jails.push_back(*pjail.get());
    }
    if (errno != 0 && errno != ENOENT)
    {
        LOG_S(ERROR) << "Error in getJails, got from GetJailParams: " << jail_errmsg;
    }
    return vect_jails;
}

std::string Tool::getLastDHCPLeaseAddress(const std::string if_name)
{
    const std::string lease_file_name = DHCP_CLIENT_LEASES_FILE + if_name;
    const std::string param_start = "lease {";
    const std::string param_addr = "fixed-address";
    const char delim_close = '}';
    std::string strbuf = "";
    std::vector<std::string> leases;
    std::vector<std::string> lease_params;
    std::vector<std::string> param;
    std::string last_lease = "";
    std::string ret_addr = "127.0.0.1";

    std::ifstream file_stream(lease_file_name);
    while (std::getline(file_stream, strbuf, delim_close))
    {
        if( std::string lease=trimString(strbuf, '\n'); !lease.empty() )
            leases.push_back(lease);
    }
    file_stream.close();
    if(!leases.empty())
    {
        last_lease = leases[leases.size()-1];
        if(!last_lease.empty())
        {
            lease_params = splitString(last_lease);
            if(lease_params.size()>1)
            {
                if(lease_params[0] == param_start)
                {
                    for(unsigned int i=1; i<lease_params.size(); i++)
                    {
                        param = splitString(leftTrimString(lease_params[i]), ' ');
                        if(param.size()>1)
                        {
                            if(param[0] == param_addr)
                            {
                                ret_addr = rightTrimString(param[1], ';');     // fixed-address 192.168.211.66;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return ret_addr;
}

std::vector<std::string> Tool::splitString(const std::string str, const char delim)
{
    std::vector<std::string> ret_vect;
    std::string strbuf;
    std::stringstream ss(str);
    while (std::getline(ss, strbuf, delim))
    {
        ret_vect.push_back(strbuf);
    }
    return ret_vect;
}

std::string Tool::leftTrimString(const std::string str, const char delim)
{
    std::string_view str_view = str;
    str_view.remove_prefix(std::min(str.find_first_not_of(delim), str.size()));
    return std::string(str_view);
}

std::string Tool::rightTrimString(const std::string str, const char delim)
{
    std::string_view str_view = str;
    str_view.remove_suffix(std::min(str.size() - str.find_last_not_of(delim) - 1, str.size()));
    return std::string(str_view);
}

std::string Tool::trimString(const std::string str, const char delim)
{
    std::string_view str_view = str;
    str_view.remove_prefix(std::min(str.find_first_not_of(delim), str.size()));
    if(str_view.empty())
        return std::string(str_view);
    str_view.remove_suffix(std::min(str.size() - str.find_last_not_of(delim) - 1, str.size()));
    return std::string(str_view);
}
