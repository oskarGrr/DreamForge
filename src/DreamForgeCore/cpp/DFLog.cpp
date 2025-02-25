#include <string>
#include "Logging.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace DF
{

Logger::Logger()
{  
    mInternalLogger = spdlog::stdout_color_mt("Internal");
    mGameAppLogger  = spdlog::stdout_color_mt("gameApp");
    spdlog::set_pattern("%^[%n %l][%T] - %v%$");
}

Logger::~Logger()
{
    spdlog::shutdown();
}

}