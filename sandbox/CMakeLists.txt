project(sandbox)

add_executable(sandbox
    sandbox.cpp
    ${ROOT_RESOURCES}
)

target_link_libraries(sandbox PRIVATE core)
target_compile_definitions(sandbox PRIVATE BUILD_SANDBOX)
