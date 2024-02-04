#include <string_view>
#include <fstream>
#include <memory>
#include <string>

#include "errorHandling.hpp"
#include "Logging.hpp"
#include "Scripting.hpp"
#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"

ScriptingEngine::ScriptingEngine()
{
    initMono();
}

ScriptingEngine::~ScriptingEngine()
{
    shutdownMono();
}

std::unique_ptr<char[]> ScriptingEngine::readCILAssemblyBytes(
    const std::string_view assemblyPath, size_t& outBufferSize)
{
    std::ifstream stream(assemblyPath.data(), std::ios::binary | std::ios::ate);
    if(!stream)
    {
        std::string errMsg {"Couldn't find the CIL assembly at\n"};
        errMsg.append(assemblyPath);
        DFLog::get().stdoutInfo(errMsg);
        return nullptr;
    }

    std::streampos end = stream.tellg();
    stream.seekg(0, std::ios::beg);
    size_t size = end - stream.tellg();
    if(size == 0)
    {
        std::string errMsg {"The file at "};
        errMsg.append(assemblyPath).append(" was empty");
        DFLog::get().stdoutInfo(errMsg);
        return nullptr;
    }

    auto buffer = std::make_unique<char[]>(size);
    stream.read(buffer.get(), size);
    outBufferSize = size;
    return buffer;
}

MonoAssembly* ScriptingEngine::LoadCILAssembly(const std::string_view assemblyPath)
{
    size_t fileSize = 0;
    auto fileData = readCILAssemblyBytes(assemblyPath, fileSize);

    //We can't use this image for anything other than loading the 
    //assembly because this image doesn't have a reference to the assembly
    MonoImageOpenStatus status;
    MonoImage* image = mono_image_open_from_data_full(fileData.get(), fileSize, 1, &status, 0);

    if(status != MONO_IMAGE_OK)
    {
        const char* errorMessage = mono_image_strerror(status);
        DFLog::get().stdoutError(errorMessage);
        return nullptr;
    }

    MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.data(), &status, false);
    mono_image_close(image);

    return assembly;
}

void ScriptingEngine::initMono()
{
#if defined(_WIN32)
    //Mono should have been installed in the Program Files folder on windows.
    mono_set_assemblies_path(
        std::string(std::getenv("ProgramFiles")).append("\\Mono\\lib").c_str());
#elif defined(__linux__)
    mono_set_assemblies_path("/lib");
#endif

    m_rootDomainPtr = mono_jit_init("j^2JitDomain");
    if(!m_rootDomainPtr)
    {
        DFLog::get().stdoutError("error durring mono_jit_init()");
    }
    
    char appDomainName[]{"j^2appDomain"};
    m_appDomainPtr = mono_domain_create_appdomain(appDomainName, nullptr);
    if(!m_appDomainPtr)
    {
        DFLog::get().stdoutError("error durring mono_domain_create_appdomain()");
    }

    mono_domain_set(m_appDomainPtr, true);
}

void ScriptingEngine::shutdownMono()
{
    mono_jit_cleanup(m_rootDomainPtr);
}

void ScriptingEngine::addInternalCall(void(*internalFunc)(), std::string_view funcName)
{
    mono_add_internal_call(funcName.data(), internalFunc);
}

void ScriptingEngine::printCILTypes(MonoAssembly* const assembly)
{
    MonoImage* image = mono_assembly_get_image(assembly);
    const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
    int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

    for(int32_t i = 0; i < numTypes; i++)
    {
        uint32_t cols[MONO_TYPEDEF_SIZE];
        mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

        const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
        const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

        printf("%s.%s\n", nameSpace, name);
    }
}