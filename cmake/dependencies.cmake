# This script defines and fetches all external dependencies using CPM.cmake.

# -----------------------------------------------------------------------------
# fmt - Modern formatting library
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY fmtlib/fmt
    VERSION 12.1.0
    GIT_TAG 12.1.0
    OPTIONS
        "FMT_TEST OFF"
        "FMT_DOC OFF"
)

# -----------------------------------------------------------------------------
# inja - Template engine for modern C++
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME inja
    GITHUB_REPOSITORY pantor/inja
    VERSION 3.5.0
    GIT_TAG v3.5.0
    OPTIONS
        "INJA_USE_EMBEDDED_JSON ON"
        "INJA_BUILD_TESTS OFF"
        "INJA_BUILD_BENCHMARKS OFF"
)

# -----------------------------------------------------------------------------
# magic_enum - Static reflection for enums
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME magic_enum
    GITHUB_REPOSITORY Neargye/magic_enum
    VERSION 0.9.7
    GIT_TAG v0.9.7
)

# -----------------------------------------------------------------------------
# ordered-map - Hash map and hash set that preserves insertion order
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME ordered-map
    GITHUB_REPOSITORY Tessil/ordered-map
    VERSION 1.2.0
    GIT_TAG v1.2.0
)

# -----------------------------------------------------------------------------
# tomlplusplus - TOML parser and serializer
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME tomlplusplus
    GITHUB_REPOSITORY marzer/tomlplusplus
    VERSION 3.4.0
    GIT_TAG v3.4.0
    OPTIONS
        "TOML_BUILD_TESTING OFF"
)

add_compile_definitions(TOML_EXCEPTIONS=0)

# -----------------------------------------------------------------------------
# yaml-cpp - YAML parser and emitter
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME yaml-cpp
    GITHUB_REPOSITORY jbeder/yaml-cpp
    VERSION 0.8.0
    GIT_TAG 0.8.0
    OPTIONS
        "YAML_CPP_BUILD_TESTS OFF"
        "YAML_CPP_BUILD_TOOLS OFF"
)

# -----------------------------------------------------------------------------
# ranges-v3 - Range-based algorithms and views for C++
# -----------------------------------------------------------------------------
CPMAddPackage(
    NAME ranges-v3
    GITHUB_REPOSITORY ericniebler/range-v3
    VERSION 0.12.0
    GIT_TAG 0.12.0
    OPTIONS
        "RANGES_BUILD_TESTS OFF"
        "RANGES_BUILD_EXAMPLES OFF"
        "RANGES_ENABLE_WARNINGS OFF"
)
