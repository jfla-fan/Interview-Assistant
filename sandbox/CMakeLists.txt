project(Sandbox)

add_executable(Sandbox
    sandbox.cpp
    ${ROOT_RESOURCES}
)

target_link_libraries(Sandbox PRIVATE core)
target_compile_definitions(Sandbox PRIVATE BUILD_SANDBOX)
