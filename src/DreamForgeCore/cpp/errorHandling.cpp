#include "errorHandling.hpp"
#include <unordered_map>
#include <string>

std::string_view DFError::getStr() const
{
    //Local static to avoid any SIOF problems where another static obj
    //needs to use this map durring its construction, but it is constructed before the map.
    static std::unordered_map<DFError::Code, std::string> errorStrings
    {
        {Code::MAX_ENTITIES_REACHED, "max entities reached"}
    };
    
    return errorStrings[m_code];
}