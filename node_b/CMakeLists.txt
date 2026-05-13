add_executable(node_b
    src/main.cpp
    src/node_b.cpp
)

target_include_directories(node_b PRIVATE src)
target_link_libraries(node_b PRIVATE common pthread)
