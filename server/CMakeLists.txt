add_executable(server
    src/main.cpp
    src/server.cpp
)

target_include_directories(server PRIVATE src)
target_link_libraries(server PRIVATE common pthread)
