#include "jailparam.h"

// Some code from /usr/src/usr.sbin/jls licensed under BSD-2-Clause-FreeBSD is used here

int JailParam::Lastjid = 0;
std::vector<jailparam> JailParam::Params;

JailParam::JailParam() : Ip4Ok(false), Ip6Ok(false), Id(0), Name(""), Ipv4Addresses(0), Ipv6Addresses(0), HostName(""), Path("") {
#ifdef INET
    Ip4Ok = (feature_present("inet") > 0);
#endif
#ifdef INET6
    Ip6Ok = (feature_present("inet6") > 0);
#endif
}

void JailParam::Print() {
    std::cout << Id << "\t" << Name << "\t";
    for(auto ipv4 : Ipv4Addresses) {
        std::cout << ipv4 << ",";
    }
    for(auto ipv6 : Ipv6Addresses) {
        std::cout << ipv6 << ",";
    }
    std::cout << "\t" << HostName << "\t" << Path << std::endl;
}

nlohmann::json JailParam::GetJailJson() {
    nlohmann::json ret_jail_json;
    nlohmann::json addr_json;
    std::vector<nlohmann::json> vect_addr_json;

    ret_jail_json[JSON_PARAM_ID] = Id;
    ret_jail_json[JSON_PARAM_NAME] = Name;
    ret_jail_json[JSON_PARAM_HOSTNAME] = HostName;
    ret_jail_json[JSON_PARAM_PATH] = Path;
    for(auto ipv4 : Ipv4Addresses) {
        addr_json[JSON_PARAM_IPV4_ADDR] = ipv4;
        vect_addr_json.push_back(addr_json);
    }
    for(auto ipv6 : Ipv6Addresses) {
        addr_json[JSON_PARAM_IPV6_ADDR] = ipv6;
        vect_addr_json.push_back(addr_json);
    }
    ret_jail_json[JSON_PARAM_ADDRESSES] = vect_addr_json;
    return ret_jail_json;
}

int JailParam::GetJailParams(int jflags)
{
    int nparams = Params.size();
    int param_size = sizeof(JailParam::ParamNames) / sizeof(std::string);
    int jid = jailparam_get((struct jailparam *)Params.data(), nparams, jflags);
    if (jid < 0)
        return jid;

    Id = *(int *)Params[0].jp_value;
    Name = std::string((char *)Params[1].jp_value);
    HostName = std::string((char *)Params[2].jp_value);
    Path = std::string((char *)Params[3].jp_value);

    if( Ip4Ok && (Params[param_size].jp_valuelen != 0) ) {
        PutIpAddrToList(AF_INET, (struct jailparam *)&Params[param_size]);
        if( Ip6Ok && (Params[param_size+1].jp_valuelen != 0) ) {
            PutIpAddrToList(AF_INET6, (struct jailparam *)&Params[param_size+1]);
        }
    } else {
        if( Ip6Ok && (Params[param_size].jp_valuelen != 0) ) {
            PutIpAddrToList(AF_INET6, (struct jailparam *)&Params[param_size]);
        }
    }
    return (jid);
}

void JailParam::PutIpAddrToList(int af_family, jailparam *param)
{
    char ipbuf[INET6_ADDRSTRLEN];
    size_t addr_len;
    int ai, count;

    switch (af_family) {
    case AF_INET:
        addr_len = sizeof(struct in_addr);
        break;
    case AF_INET6:
        addr_len = sizeof(struct in6_addr);
        break;
    default:
        LOG_S(ERROR) << "ERROR in JailParam::PutIpAddrToList: unsupported af_family";
        return;
    }
    count = param->jp_valuelen / addr_len;
    for (ai = 0; ai < count; ai++) {
        if (inet_ntop(af_family, ((uint8_t *)param->jp_value) + addr_len * ai, ipbuf, sizeof(ipbuf)) != NULL) {
            switch (af_family) {
            case AF_INET:
                Ipv4Addresses.push_back(std::string(ipbuf));
                break;
            case AF_INET6:
                Ipv6Addresses.push_back(std::string(ipbuf));
                break;
            }
        }
    }
}

void JailParam::InitRequestParams() {
    bool ip4_ok = false;
    bool ip6_ok = false;
#ifdef INET
    ip4_ok = (feature_present("inet") > 0);
#endif
#ifdef INET6
    ip6_ok = (feature_present("inet6") > 0);
#endif
    Lastjid = 0;
    Params.clear();
    for(auto p : ParamNames) {
        AddParam(p.c_str(), NULL, (size_t)0, NULL, JailParam::JP_USER);
    }
    if(ip4_ok) {
        AddParam(ParamIpv4.c_str(), NULL, (size_t)0, NULL, JailParam::JP_USER);
    }
    if(ip6_ok) {
        AddParam(ParamIpv6.c_str(), NULL, (size_t)0, NULL, JailParam::JP_USER | JailParam::JP_OPT);
    }
    AddParam(ParamLastJid.c_str(), &Lastjid, sizeof(Lastjid), NULL, 0);
}

int JailParam::AddParam(const char *name, void *value, size_t valuelen, jailparam *source, unsigned flags)
{
    struct jailparam par;
    memset(&par, 0, sizeof(par));

    if (source != NULL) {
        memcpy(&par, source, sizeof(par));
        par.jp_flags |= flags;
        Params.push_back(par);
        return Params.size();
    }
    if ( jailparam_init(&par, name) < 0 ||
         ( value != NULL ? jailparam_import_raw(&par, value, valuelen) : jailparam_import(&par, (const char*)value) ) < 0 ) {
        if (flags & JailParam::JP_OPT) {
            return (-1);
        }
        LOG_S(ERROR) << "ERROR in JailParam::AddParam got from jailparam_import_raw or jailparam_import: " << jail_errmsg;
    }
    par.jp_flags |= flags;
    Params.push_back(par);
    return Params.size();
}
