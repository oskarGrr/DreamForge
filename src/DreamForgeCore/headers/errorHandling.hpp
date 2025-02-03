#pragma once
#include <expected>//C++23 and beyond
#include <optional>
#include <cstdint>
#include <string_view>
#include <string>
#include <exception>
#include <source_location>
#include <stacktrace>

#include "HelpfulTypeAliases.hpp"

#define STRINGIZE(x) #x
#define EXPAND_STRINGIZE(x) STRINGIZE(x)

//The current line of source code as a string
#define LINESTR EXPAND_STRINGIZE(__LINE__)

#ifdef TOP_LEVEL_CMAKE
//Absolute path of the top level CMakeLists.txt as a string literal.
#define TOP_LEVEL_CMAKE_STR EXPAND_STRINGIZE(TOP_LEVEL_CMAKE)
#endif

namespace DF
{

class Error;

template <typename ExpectedType>
using Expect = std::expected<ExpectedType, Error>;

//Basically just a nullable reference.
//I would just use raw pointers, but I figured this offers stronger semantics.
//Now instead of a raw pointer which could imply ownership, this is more clear.
template <typename T>
using NonOwningPtr = T*;

using MaybeError = std::optional<Error>;

//Could be used as the error part of std::expected, or just used directly.
class [[nodiscard("don't forget your error code!")]] Error
{
public:

    //Types of error codes.
    enum struct [[nodiscard("don't forget your error code!")]] Code : uint8_t
    {
        NONE = 0,
        MAX_ENTITIES_REACHED,
        COULD_NOT_COMPILE_SHADERS
    };

    Error(Code code) : m_code{code} {}

    Code getErrCode() const {return m_code;}
    std::string_view getStr() const;

private:
    Code m_code;
};

class DFException : public std::exception
{
public:
    DFException(std::string_view errInfo, 
        std::optional<S64> possibleErrorCode = std::nullopt)
        : mPossibleErrorCode{possibleErrorCode}, mErrorInfo{errInfo} {}

    virtual ~DFException()=default;

    [[nodiscard]] char const* what() const override;
    auto getPossibleErrCode() const {return mPossibleErrorCode;}

protected:
    std::string mErrorInfo;
    std::optional<S64> mPossibleErrorCode;
    std::source_location mLocation {std::source_location::current()};
    std::stacktrace mStackTrace {std::stacktrace::current()};
};

class SystemInitException : public DFException
{
public:
    SystemInitException(std::string_view exceptionInfo, std::optional<S64> possibleErrorCode = std::nullopt)
        : DFException{exceptionInfo, possibleErrorCode} {}

    virtual ~SystemInitException()=default;
};

void errorHandlerCallbackglfw(int code, const char* codeStr);

}//end namespace DF