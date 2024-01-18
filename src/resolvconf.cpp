#include "resolvconf.h"

ResolvConf::ResolvConf(std::string path, short nbkp) : confFileName(path), nBackups(nbkp)
{}

bool ResolvConf::rotateConfFile()
{
    return Tool::rotateConfigFile(confFileName, nBackups);
}

json ResolvConf::getConfig()
{
    std::ifstream conf_file;
    std::string line;
    std::string subline;
    std::string value;
    std::shared_ptr<AddressIp4> addr4=nullptr;
    std::vector<std::shared_ptr<AddressIp4>> nameservers;
    std::vector<std::string> search_domains;
    std::string domain;
    unsigned long pos = -1;
    std::string_view line_view;
    json jarr_nameservers = json::array();
    json jarr_srchdomains = json::array();
    json jdata {};
    json jret {};

    conf_file.open(confFileName);
    if(!conf_file.is_open())
    {
        LOG_S(ERROR) << "getConfig cannot open configuration file " << confFileName;
        return JSON_RESULT_ERR;
    }

    while (getline(conf_file, line))
    {
        line_view = line;
        line_view.remove_prefix(std::min(line.find_first_not_of(" "), line.size()));
        line_view.remove_suffix(std::min(line.size() - line.find_last_not_of(" ") - 1, line.size()));
        if(line_view.starts_with('#'))
            continue;
        else if(line_view.starts_with(NAMESERVER))
        {
            pos = line_view.find_first_of(" ");
            if( (pos != std::string::npos) && (pos >= NAMESERVER.length()) )
            {
                value = line_view.substr(pos+1);
                try
                {
                    addr4 = std::make_shared<AddressIp4>(value);
                    nameservers.push_back(addr4);
                }
                catch (std::exception& e)
                {
                    LOG_S(WARNING) << "getConfig cannot read NAMESERVER data from " << line_view;
                }
            }
            continue;
        }
        else if(line_view.starts_with(DOMAIN))
        {
            pos = line_view.find_first_of(" ");
            if( (pos != std::string::npos) && (pos >= DOMAIN.length()) )
            {
                domain = line_view.substr(pos+1);
            }
            continue;
        }
        else if(line_view.starts_with(SEARCH))
        {
            pos = line_view.find_first_of(" ");
            if( (pos != std::string::npos) && (pos >= SEARCH.length()) )
            {
                value = line_view.substr(pos+1);
                auto ss = std::stringstream(value);
                while (getline(ss, subline, ' '))
                {
                    if(!subline.empty())
                        search_domains.push_back(subline);
                }
            }
            continue;
        }
    }
    conf_file.close();
    for(auto ns : nameservers)
    {
        value = ns->getStrAddr();
        jarr_nameservers.push_back(value);
    }
    for(auto dom : search_domains)
    {
        jarr_srchdomains.push_back(dom);
    }
//    const std::string JSON_PARAM_NAMESERVERS = "NAMESERVERS";
//    const std::string JSON_PARAM_SEARCH_DOMAINS = "SEARCH DOMAINS";
//    const std::string JSON_PARAM_HOST_DOMAIN = "DOMAIN";
    jdata[JSON_PARAM_NAMESERVERS] = jarr_nameservers;
    jdata[JSON_PARAM_SEARCH_DOMAINS] = jarr_srchdomains;
    jdata[JSON_PARAM_HOST_DOMAIN] = domain;
    jret[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    jret[JSON_PARAM_DATA] = jdata;
    return jret;
}

bool ResolvConf::setConfig(json data)
{
/*
{
    "SCOPE" : "SYSTEM",
    "CMD" : "RESOLVCONF_WRITE",
    "DATA": {
        "DOMAIN": "domain1.local",
        "NAMESERVERS": [
          "8.8.8.8",
          "4.4.4.4"
        ],
        "SEARCH DOMAINS": [
          "domain1.local",
          "domain2.local"
        ]
    }
}
*/
    std::string domain = "";
    json jar_nameservers = json::array();
    std::vector<std::string> nameservers;
    json jar_search_domains = json::array();
    std::vector<std::string> search_domains;
    try
    {
        domain = data[JSON_PARAM_HOST_DOMAIN];
        jar_nameservers = data[JSON_PARAM_NAMESERVERS];
        jar_search_domains = data[JSON_PARAM_SEARCH_DOMAINS];
    }
    catch (std::exception& e) {
        LOG_S(ERROR) << "ResolvConf::setConfig - cannot get resolv.conf parameters from json: " << e.what();
        return false;
    }
    for(const auto &ns : jar_nameservers)
    {
        nameservers.push_back(ns);
    }
    for(const auto &sd : jar_search_domains)
    {
        search_domains.push_back(sd);
    }
    try
    {
        LOG_S(INFO) << "Writing to " << confFileName << std::endl;
        std::ofstream resolconf_file(confFileName, std::ios::trunc);
        resolconf_file << DOMAIN << " " << domain << std::endl;
        if(!search_domains.empty())
        {
            resolconf_file << SEARCH;
            for(const auto &sd : search_domains)
            {
                resolconf_file << " " << sd;
            }
            resolconf_file << std::endl;
        }
        if(!nameservers.empty())
        {
            for(const auto &ns : nameservers)
            {
                resolconf_file << NAMESERVER << " " << ns << std::endl;
            }
        }
        resolconf_file << std::endl;
        resolconf_file.close();
    }
    catch (std::exception& e) {
        LOG_S(ERROR) << "ResolvConf::setConfig - cannot write to " << confFileName << " : " << e.what();
        return false;
    }
    return true;
}
