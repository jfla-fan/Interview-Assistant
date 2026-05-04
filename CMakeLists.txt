cmake_minimum_required(VERSION 3.16)

project(InterviewAssistant LANGUAGES CXX VERSION 1.0.0.0)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BINARY_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(PROJECT_ROOT      ${CMAKE_CURRENT_SOURCE_DIR})
set(RESOURCES_DIR     ${PROJECT_ROOT}/resources)
set(PACKAGING_DIR     ${PROJECT_ROOT}/packaging/windows)

# Apply to all configurations (single-config & multi-config generators)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY                  ${BINARY_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG            ${BINARY_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE          ${BINARY_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO   ${BINARY_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL       ${BINARY_OUTPUT_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# options
option(ENABLE_QT_LOG_HOOKING "Enable Qt logging integration" ON)
option(BUILD_SANDBOX "Build Sandbox." ON)

if(ENABLE_QT_LOG_HOOKING)
    add_compile_definitions(ENABLE_QT_LOG_HOOK)
else()
    message(STATUS "Qt logging integration is disabled.")
endif()

# other dependencies

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Concurrent)

include(CPM)
include(dependencies)

add_subdirectory(third_party/QHotkey)
add_subdirectory(third_party/QSourceHighlighter)

message("Runtime output directory from root: < ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} >")

file(GLOB_RECURSE ROOT_RESOURCES
    "${RESOURCES_DIR}/*.qrc"
)

get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

configure_file(
    core/version.h.in
    ${PROJECT_ROOT}/core/version.h
    @ONLY
)

configure_file(
  packaging/windows/deploy.py.in
  ${CMAKE_BINARY_DIR}/deploy.py
  @ONLY
)

# core library
add_subdirectory(core)

# sandbox
if (BUILD_SANDBOX)
    message(STATUS "Building sandbox")
    add_subdirectory(sandbox)
endif()

# main application
add_subdirectory(app)
