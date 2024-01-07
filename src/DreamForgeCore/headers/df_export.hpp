#pragma once

#if (defined(_MSC_VER) || defined(__MINGW32__))
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)//GCC/clang/ICC
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

#ifdef DF_DLL_INTERNAL
    #define DF_DLL_API EXPORT
#else
    #define DF_DLL_API IMPORT
#endif