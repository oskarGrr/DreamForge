set(HEADER_FILES
    headers/GUITheme.hpp
)

set(CPP_FILES
    cpp/main.cpp
	cpp/GUITheme.cpp
)

add_executable(${EDITOR_EXE_NAME}
    ${CPP_FILES}
    ${HEADER_FILES}
)

target_link_libraries(${EDITOR_EXE_NAME} PRIVATE ${CORE_LIB_NAME})

source_group(hpp FILES ${HEADER_FILES})
source_group(cpp FILES ${CPP_FILES})

target_include_directories(${EDITOR_EXE_NAME} PRIVATE headers)

add_custom_target(copy_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
	"${CMAKE_SOURCE_DIR}/../resources" "$<TARGET_FILE_DIR:${EDITOR_EXE_NAME}>/resources"
)
add_dependencies(${EDITOR_EXE_NAME} copy_resources)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    add_custom_command(TARGET ${EDITOR_EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{ProgramFiles}\\Mono\\bin\\mono-2.0-sgen.dll"
        $<TARGET_FILE_DIR:${EDITOR_EXE_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()