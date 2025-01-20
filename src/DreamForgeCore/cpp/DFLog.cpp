#include <string>
#include "Logging.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace DF
{

Logger::Logger()
{  
    m_internalLogger = spdlog::stdout_color_mt("Internal");
    m_gameAppLogger  = spdlog::stdout_color_mt("gameApp");
    spdlog::set_pattern("%^[%n %l][%T] - %v%$");
}

Logger::~Logger()
{
    spdlog::shutdown();
}

}