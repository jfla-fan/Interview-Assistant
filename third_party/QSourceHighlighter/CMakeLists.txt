cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(QSourceHighlighter
    qsourcehighliter.cpp
    languagedata.cpp
    qsourcehighliterthemes.cpp
)

# Set include directory
target_include_directories(QSourceHighlighter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/QSourceHighlighter>
)

# Link Qt dependencies
target_link_libraries(QSourceHighlighter
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# C++11 or higher
target_compile_features(QSourceHighlighter PUBLIC cxx_std_11)

# Optional: Set alias for convenience
add_library(QSourceHighlighter::QSourceHighlighter ALIAS QSourceHighlighter)
