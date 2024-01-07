#include <string>
#include "Logging.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

DFLog::DFLog()
{  
    m_internalLogger = spdlog::stdout_color_mt("Internal");
    m_gameAppLogger  = spdlog::stdout_color_mt("gameApp");
    spdlog::set_pattern("%^[%n %l][%T] - %v%$");
}

DFLog::~DFLog()
{
    spdlog::shutdown();
}