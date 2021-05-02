#include "rcconf.h"

rcconf::rcconf(std::string path) : rcFileName(path)
{
    rcIniFile = new CIniFile();
}

rcconf::~rcconf()
{
    delete rcIniFile;
}

bool rcconf::iniLoad()
{
    return rcIniFile->Load(rcFileName);
}

json rcconf::getRcIpConfig()
{
    std::string strDefaultRouter = "";
    std::string key = "";
    std::string value = "";
    std::string_view value_view;
    std::string element;
    std::string_view element_view;
    std::stringstream ss;
    const char DELIM = ' ';
    json jret {};
    std::map<std::string, std::string> mapInterfaces;
    std::map<std::string, std::string> mapRoutes;
    std::set<std::string> setActiveRoutes;

    for( SecIndex::const_iterator itr = rcIniFile->GetSections().begin(); itr != rcIniFile->GetSections().end(); ++itr )
    {
        for( KeyIndex::const_iterator kitr = (*itr)->GetKeys().begin(); kitr != (*itr)->GetKeys().end(); kitr++ )
        {
            key = (*kitr)->GetKeyName();
            value = (*kitr)->GetValue();
            if(key==DEFAULT_ROUTE_KEY)
            {
                LOG_S(INFO) << "getRcIpConfig: found " << key << " : " << value;
                strDefaultRouter = value;
                continue;
            }
            if(key==ROUTES_KEY)
            {
                LOG_S(INFO) << "getRcIpConfig: found " << key << " : " << value;
                ss = std::stringstream(value);
                while (getline (ss, element, DELIM))
                {
                    element_view = element;
                    element_view.remove_prefix(std::min(element.find_first_not_of(" "), element.size()));
                    element_view.remove_suffix(std::min(element.size() - element.find_last_not_of(" ") - 1, element.size()));
                    setActiveRoutes.insert(static_cast<std::string>(element_view));
                }
                LOG_S(INFO) << "getRcIpConfig: found " << setActiveRoutes.size() << " active routes";
                continue;
            }
            value_view = value;
            if(value_view.starts_with(IFCONFIG_KEY_PREFIX))
            {
                continue;
            }
            if(value_view.starts_with(ROUTE_KEY_PREFIX))
            {
                continue;
            }
        }
    }
    return jret;
}

bool rcconf::setRcIpConfig(json*)
{
    return true;
}
