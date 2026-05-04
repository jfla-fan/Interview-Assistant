project(core)

add_library(core STATIC
                 action.h
                 common.h
                 config_manager.h
                 config_manager.cpp
                 hotkey_manager.h
                 hotkey_manager.cpp
                 hotkey_manager_p.h
                 launcher.h
                 launcher.cpp
                 launcher_p.h
                 screenshot_helper.h
                 screenshot_helper.cpp

                 widgets/private/proxy_table_widget.h
                 widgets/private/proxy_table_widget.cpp
                 widgets/private/hotkey_table_widget.h
                 widgets/private/hotkey_table_widget.cpp
                 widgets/private/service_config_text_edit.h
                 widgets/private/service_config_text_edit.cpp

                 widgets/markdown_viewer.h
                 widgets/markdown_viewer.cpp
                 widgets/settings.h
                 widgets/settings.cpp
                 widgets/request_info_menu.h
                 widgets/request_info_menu.cpp
                 widgets/interactive_label.h
                 widgets/interactive_label.cpp
                 widgets/overlay_widget.h
                 widgets/overlay_widget.cpp
                 widgets/log_panel.h
                 widgets/log_panel.cpp
                 widgets/chat_window.h
                 widgets/chat_window.cpp
                 widgets/chat_message_widget.h
                 widgets/chat_message_widget.cpp
                 widgets/overlay_menu.h
                 widgets/overlay_menu.cpp
                 widgets/screenshot_tool.h
                 widgets/screenshot_tool.cpp
                 widgets/file_list_menu.h
                 widgets/file_list_menu.cpp
                 widgets/base_widget.h
                 widgets/base_widget.cpp
                 widgets/private/base_widget_impl.h
                 widgets/private/base_widget_impl.cpp
                 widgets/i_base_widget.h

                 network/api_controller.h
                 network/api_controller.cpp
                 network/api_controller_p.h
                 network/proxy_checker.h
                 network/proxy_checker.cpp

                 highlighters/markdown_highlighter.h

                 log/log.h
                 log/log.cpp
                 log/logger_thread.h
                 log/logger_thread.cpp
                 log/formatters/fmt.h
                 log/formatters/fmt.cpp
                 log/log_component.h
                 log/log_component.cpp
                 log/qt_log_hook.inl
                 log/log_formatter.h
                 log/components/console.h
                 log/components/console.cpp
                 log/components/colorful_console.h
                 log/components/colorful_console.cpp
                 log/components/rotating_file.h
                 log/components/rotating_file.cpp
                 log/components/qt_text_edit.h
                 log/components/qt_text_edit.cpp

                 config/app_config.h
                 config/app_config.cpp
                 config/app_config_fwd.h
                 config/service_config.cpp
                 config/service_config.h
                 config/service_config_fwd.h
                 config/service_config_loader.h
                 config/service_config_loader.cpp
                 config/yaml_parser.h
                 config/yaml_parser.cpp
                 config/yaml_parser_p.h
                 config/inja_renderer.h
                 config/inja_renderer.cpp

                 utils/files.h
                 utils/files.cpp
                 utils/platform.h
                 utils/platform.cpp
                 utils/numeric.h
                 utils/containers.h
                 utils/style.h
                 utils/style.cpp

                # packaging/windows/resource.h
)


target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)


target_link_libraries(core  PUBLIC
                            Qt${QT_VERSION_MAJOR}::Core
                            Qt${QT_VERSION_MAJOR}::Widgets
                            Qt${QT_VERSION_MAJOR}::Network
                            Qt${QT_VERSION_MAJOR}::Concurrent
                            QHotkey::QHotkey
                            yaml-cpp::yaml-cpp
                            inja
                            tsl::ordered_map
                            tomlplusplus::tomlplusplus
                            magic_enum::magic_enum
                            fmt::fmt
                            range-v3::range-v3
                            QSourceHighlighter::QSourceHighlighter)


set_target_properties(core PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    POSITION_INDEPENDENT_CODE ON
)

message("Runtime output directory from core: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
