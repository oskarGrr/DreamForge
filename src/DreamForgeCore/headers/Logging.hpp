#pragma once
#include <string_view>
#include <memory>
#include "df_export.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"

//Singleton logger. Avoids SIOF problems by having a local static
//inside of get() instead of a class or global static. This is important because
//another static object might want to log something durring construction and it's
//constuction might take place fist before Logger's leading to UB. 
//So by delaying Logger's construction until get() is called the first time,
//I can avoid any static initialization order fiascos :)

namespace DF 
{

class DF_DLL_API Logger
{
public:
    static Logger& get()
    {
        static Logger logger;
        return logger;
    }
private:
    Logger();
    ~Logger();
    
    Logger(Logger const&)=delete;
    Logger& operator=(Logger const&)=delete;
    Logger(Logger&&)=delete;
    Logger& operator=(Logger&&)=delete;

    //The logger used from within the engine DLL.
    std::shared_ptr<spdlog::logger> m_internalLogger{nullptr};
    
    //The logger used by the application linking with the DLL.
    std::shared_ptr<spdlog::logger> m_gameAppLogger{nullptr};
    
    spdlog::filename_t m_logFilePath { std::string{"./DFLog.txt"} };
public:

//stdoutxxx() logs messages for every param by folding
//the parameter pack over the coma operator. This calls info() 
//for every message in the messages parameter pack.
//fmtStdoutxxx() logs a printf style format string.
#ifdef DF_DLL_INTERNAL
    void stdoutInfo(auto const&... messages){
        (m_internalLogger->info(messages), ...);
    }
    void stdoutWarn(auto const&... messages){
        (m_internalLogger->warn(messages), ...);
    }
    void stdoutError(auto const&... messages){
        (m_internalLogger->error(messages), ...);
    }

    template <class ...Args> void 
    fmtStdoutInfo(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        m_internalLogger->info(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutWarn(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        m_internalLogger->warn(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutError(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        m_internalLogger->error(fmtStr, std::forward<Args>(toFormat)...);
    }
#else
    void stdoutInfo(auto const&... messages){
        (m_gameAppLogger->info(messages), ...);
    }
    void stdoutWarn(auto const&... messages){
        (m_gameAppLogger->warn(messages), ...);
    }
    void stdoutError(auto const&... messages){
        (m_gameAppLogger->error(messages), ...);
    }

    template <class ...Args> 
    void fmtStdoutInfo(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        m_gameAppLogger->info(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutWarn(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        m_gameAppLogger->warn(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutError(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        m_gameAppLogger->error(fmtStr, std::forward<Args>(toFormat)...);
    }
#endif
};

}//end namespace DF