#include "nmconfig.h"

nmconfig::nmconfig(std::string filename) : confFileName(filename), confIniFile()
{
}

nmconfig::~nmconfig()
{
    delete confIniFile;
}

bool nmconfig::iniLoad()
{
    return confIniFile->Load(confFileName);
}

bool nmconfig::isValidSection(std::string section)
{
    for(auto s : sections)
    {
        if(section == s)
            return true;
    }
    LOG_S(ERROR) << "Invalid config file section: " << section;
    return false;
}

std::string nmconfig::getConfigValue(std::string section, std::string key)
{
    std::string ret_val = "";

    if(!isValidSection(section))
        return ret_val;

    ret_val = confIniFile->GetKeyValue(section, key);
    return ret_val;
}
