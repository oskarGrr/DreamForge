set(HEADER_FILES

)

add_executable(${EDITOR_EXE_NAME}
    cpp/main.cpp
)

target_link_libraries(${EDITOR_EXE_NAME} PRIVATE ${CORE_LIB_NAME})

source_group(headers FILES ${HEADER_FILES})

target_include_directories(${EDITOR_EXE_NAME} PRIVATE headers)

add_custom_target(copy_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
	"${CMAKE_SOURCE_DIR}/../resources" "${CMAKE_CURRENT_BINARY_DIR}/resources"
)
add_dependencies(${EDITOR_EXE_NAME} copy_resources)

add_custom_target(copy_mono
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
	"${CMAKE_SOURCE_DIR}/../thirdPartyPackages/mono" "${CMAKE_CURRENT_BINARY_DIR}/mono"
)
add_dependencies(${EDITOR_EXE_NAME} copy_mono)

add_custom_command(TARGET ${EDITOR_EXE_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${CMAKE_SOURCE_DIR}/../thirdPartyPackages/mono/bin/mono-2.0-sgen.dll
	$<TARGET_FILE_DIR:${EDITOR_EXE_NAME}>
	COMMAND_EXPAND_LISTS
)