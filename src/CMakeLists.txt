cmake_minimum_required(VERSION 3.21)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../buildOutput/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../buildOutput/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../buildOutput/lib)

if(EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_TARGET_ARCHITECTURE x64)
	set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/../thirdPartyPackages/vcpkg_installed")
else()
    message(FATAL_ERROR 
        "vcpkg could not be found. If you already have vcpkg installed,
		then set the environment variable VCPKG_ROOT to the absolute path
		where vcpkg was installed. For some reason vcpkg does not set this 
		environment variable automatically when it gets installed."
    )
endif()

project(DreamForge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

set(CORE_LIB_NAME ${PROJECT_NAME}Core)
set(EDITOR_EXE_NAME ${PROJECT_NAME}Editor)

add_subdirectory(${CORE_LIB_NAME})
add_subdirectory(${EDITOR_EXE_NAME})

#install(TARGETS ${CORE_LIB_NAME} 
#    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/../install/bin
#    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/../install/lib
#    ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/../install/lib
#)
#
#install(TARGETS ${CORE_EXE_NAME} DESTINATION ${CMAKE_SOURCE_DIR}/../install/bin)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_EXE_NAME})