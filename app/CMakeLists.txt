project(app)


add_executable(InterviewAssistant WIN32
    main.cpp
    ${ROOT_RESOURCES}
    ${PACKAGING_DIR}/resources.rc
)

target_link_libraries(InterviewAssistant PRIVATE core)

target_compile_definitions(InterviewAssistant PRIVATE BUILD_APP)

include(GNUInstallDirs)
install(TARGETS InterviewAssistant
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message("Runtime output directory from app: ${CMAKE_CURRENT_BINARY_DIR}")
