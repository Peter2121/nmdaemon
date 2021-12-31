#include "nmconfig.h"

NmConfig::NmConfig(std::string filename) : confFileName(filename)
{
    confIniFile = new CIniFile();
}

NmConfig::~NmConfig()
{
    delete confIniFile;
}

bool NmConfig::iniLoad()
{
    return confIniFile->Load(confFileName);
}

bool NmConfig::isValidSection(std::string section) const
{
    for(auto s : sections)
    {
        if(section == s)
            return true;
    }
    LOG_S(ERROR) << "Invalid config file section: " << section;
    return false;
}

std::string NmConfig::getConfigValue(std::string section, std::string key) const
{
    std::string ret_val = "";

    if(!isValidSection(section))
        return ret_val;

    ret_val = confIniFile->GetKeyValue(section, key);
    return ret_val;
}

std::string NmConfig::getConfigFileName() const
{
    return confFileName;
}
