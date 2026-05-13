add_executable(node_a
    src/main.cpp
    src/node_a.cpp
)

target_include_directories(node_a PRIVATE src)
target_link_libraries(node_a PRIVATE common pthread)
