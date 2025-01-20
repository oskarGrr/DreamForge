#pragma once
#include <expected>//C++23 and beyond
#include <functional>//std::reference_wrapper
#include <optional>
#include <cstdint>
#include <string_view>

#define STRINGIZE(x) #x
#define EXPAND_STRINGIZE(x) STRINGIZE(x)

//The current line of source code as a string
#define LINESTR EXPAND_STRINGIZE(__LINE__)

//Absolute path of the top level CMakeLists.txt as a string literal.
#define TOP_LEVEL_CMAKE_STR EXPAND_STRINGIZE(TOP_LEVEL_CMAKE)

namespace DF
{

class Error;

template <typename ExpectedType>
using Expect = std::expected<ExpectedType, Error>;

template <typename OptionalType>
using DFMaybeRef = std::optional< std::reference_wrapper<OptionalType> >;


//Could be used as the error part of std::expected, or just used directly.
class [[nodiscard("don't forget your error code!")]] Error
{
public:

    //Types of error codes.
    enum struct [[nodiscard("don't forget your error code!")]] Code : uint8_t
    {
        MAX_ENTITIES_REACHED
    };

    Error(Code code) : m_code{code} {}

    Code getErrCode() const {return m_code;}
    std::string_view getStr() const;

private:
    Code m_code;
};

void errorHandlerCallbackglfw(int code, const char* codeStr);

}

