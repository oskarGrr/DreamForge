#include "errorHandling.hpp"
#include "Logging.hpp"
#include <unordered_map>
#include <string>


namespace DF
{

std::string_view Error::getStr() const
{
    //Local static to avoid any SIOF problems where another static obj
    //needs to use this map durring its construction, but it is constructed before the map.
    static std::unordered_map<Error::Code, std::string> errorStrings
    {
        {Code::MAX_ENTITIES_REACHED, "max entities reached"},
        {Code::COULD_NOT_COMPILE_SHADERS, "could not compile shaders"}
    };
    
    return errorStrings[m_code];
}

void errorHandlerCallbackglfw(int code, const char* codeStr)
{
    //for now just log the error. if necessary, in the future 
    //do specific things depending on the error code.
    Logger::get().stdoutError(codeStr);
}

char const* DFException::what() const
{
    std::string exceptInfo {mErrorInfo};
    return nullptr;
}

}