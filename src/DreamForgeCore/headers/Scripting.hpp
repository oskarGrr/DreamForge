#pragma once

#include "mono/metadata/image.h"
#include "mono/utils/mono-forward.h"
#include <memory>//unique_ptr

namespace DF
{

//Not a static singleton to avoid SIOF and other problems, but
//there is still only meant to be 1 instantiation of a ScriptingEngine.
class ScriptingEngine
{
public:
    ScriptingEngine();
    ~ScriptingEngine();

    void addInternalCall(const void* method, std::string_view funcName);
    static void printCILTypes(MonoAssembly* const assembly);
    [[nodiscard]] static MonoAssembly* LoadCILAssembly(const std::string_view assemblyPath);

private:
    void initMono();
    void shutdownMono();
    static std::unique_ptr<char[]> readCILAssemblyBytes(
        const std::string_view assemblyPath, size_t& outBufferSize);

    MonoDomain*   mRootDomainPtr{nullptr};
    MonoDomain*   mAppDomainPtr{nullptr};
    MonoAssembly* mCsharpEngineAPIAssembly{nullptr};
};

}