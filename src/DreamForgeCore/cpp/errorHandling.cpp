#include "errorHandling.hpp"
#include "Logging.hpp"
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

void errorHandlerCallbackglfw(int code, const char* codeStr)
{
    //for now just log the error. if necessary, in the future 
    //do specific things depending on the error code.
    DFLog::get().stdoutError(codeStr);
}