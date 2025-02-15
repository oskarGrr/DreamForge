set(HEADER_FILES 
    headers/BiDirectionalTypeIntMap.hpp
    headers/df_export.hpp
    headers/errorHandling.hpp
    headers/HelpfulTypeAliases.hpp
    headers/Logging.hpp
    headers/Scripting.hpp
    headers/Window.hpp
    interfaceHeaders/DreamForge.hpp
	headers/VulkanDevice.hpp
	headers/VulkanRenderer.hpp
	headers/ApplicationBase.hpp
)

set(CPP_FILES
    cpp/errorHandling.cpp
    cpp/DFLog.cpp
    cpp/Scripting.cpp
    cpp/Window.cpp
	cpp/VulkanDevice.cpp
	cpp/VulkanRenderer.cpp
	cpp/ApplicationBase.cpp
)

set(ECS_SRC_FILES
    cpp/ECS/ComponentManager.cpp
    cpp/ECS/ComponentManager.hpp
    cpp/ECS/Components.hpp
    cpp/ECS/ECS.cpp
    cpp/ECS/ECS.hpp
    cpp/ECS/ECSEvents.hpp
    cpp/ECS/EntityManager.cpp
    cpp/ECS/EntityManager.hpp
)

add_library(${CORE_LIB_NAME} SHARED 
    ${CPP_FILES}
    ${HEADER_FILES}
    ${ECS_SRC_FILES}
)

source_group(hpp FILES ${HEADER_FILES})
source_group(ECS FILES ${ECS_SRC_FILES})
source_group(cpp FILES ${CPP_FILES})

#TODO For now the headers for the core dll will be public until I get around to sorting
#them into interface/public/private folders. Some of the headers that will end up in the
#public/shared folder will not neccesarily be meant to be directly included by the application,
#but rather included indirectly through an interface header (see ./interfaceHeaders/DreamForge.hpp).
target_include_directories(${CORE_LIB_NAME} PUBLIC headers)
target_include_directories(${CORE_LIB_NAME} PUBLIC cpp/ECS)

#Generates a #define DF_DEBUG when in debug builds.
target_compile_definitions(${CORE_LIB_NAME} PRIVATE $<$<CONFIG:Debug>:DF_DEBUG>)

#Used in headers that are shared with the application to enable/disable code
#depending on if the header is #included from the app or the dll.
target_compile_definitions(${CORE_LIB_NAME} PRIVATE DF_DLL_INTERNAL)

#target_compile_definitions(${CORE_LIB_NAME} PRIVATE -DTOP_LEVEL_CMAKE=${CMAKE_SOURCE_DIR})

target_include_directories(${CORE_LIB_NAME} INTERFACE interfaceHeaders)

set(CMAKE_FIND_DEBUG_MODE True)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${CORE_LIB_NAME} PUBLIC imgui::imgui)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${CORE_LIB_NAME} PRIVATE glm::glm)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${CORE_LIB_NAME} PUBLIC spdlog::spdlog_header_only)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(${CORE_LIB_NAME} PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(${CORE_LIB_NAME} PRIVATE Vulkan::Vulkan)

find_package(unofficial-shaderc CONFIG REQUIRED)
target_link_libraries(${CORE_LIB_NAME} PRIVATE unofficial::shaderc::shaderc)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    find_library(MONO_RUNTIME mono-2.0-sgen.lib REQUIRED HINTS "$ENV{ProgramFiles}\\Mono\\lib")
	target_include_directories(${CORE_LIB_NAME} PRIVATE "$ENV{ProgramFiles}\\Mono\\include\\mono-2.0")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    find_library(MONO_RUNTIME monosgen-2.0 REQUIRED HINTS "/lib" "/usr/lib")
	target_include_directories(${CORE_LIB_NAME} PRIVATE "/usr/include/mono-2.0")
endif()

target_link_libraries(${CORE_LIB_NAME} PRIVATE ${MONO_RUNTIME})