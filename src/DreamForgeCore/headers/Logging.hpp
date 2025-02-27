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

    //The logger that is used from within the engine DLL.
    std::shared_ptr<spdlog::logger> mInternalLogger{nullptr};
    
    //The logger that is used by the application linking with the DLL.
    std::shared_ptr<spdlog::logger> mGameAppLogger{nullptr};
    
    spdlog::filename_t m_logFilePath { std::string{"./DFLog.txt"} };

public:

//stdoutxxx() logs messages for every param by folding
//the parameter pack over the coma operator. This calls info() 
//for every message in the messages parameter pack.
//fmtStdoutxxx() logs a printf style format string.
#ifdef DF_DLL_INTERNAL
    void stdoutInfo(auto const&... messages){
        (mInternalLogger->info(messages), ...);
    }
    void stdoutWarn(auto const&... messages){
        (mInternalLogger->warn(messages), ...);
    }
    void stdoutError(auto const&... messages){
        (mInternalLogger->error(messages), ...);
    }

    template <class ...Args> void 
    fmtStdoutInfo(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        mInternalLogger->info(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutWarn(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        mInternalLogger->warn(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutError(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        mInternalLogger->error(fmtStr, std::forward<Args>(toFormat)...);
    }
#else
    void stdoutInfo(auto const&... messages){
        (mGameAppLogger->info(messages), ...);
    }
    void stdoutWarn(auto const&... messages){
        (mGameAppLogger->warn(messages), ...);
    }
    void stdoutError(auto const&... messages){
        (mGameAppLogger->error(messages), ...);
    }

    template <class ...Args> 
    void fmtStdoutInfo(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        mGameAppLogger->info(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutWarn(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        mGameAppLogger->warn(fmtStr, std::forward<Args>(toFormat)...);
    }
    template <class ...Args> 
    void fmtStdoutError(spdlog::format_string_t<Args...> fmtStr, Args&&... toFormat){
        mGameAppLogger->error(fmtStr, std::forward<Args>(toFormat)...);
    }
#endif
};

}//end namespace DF