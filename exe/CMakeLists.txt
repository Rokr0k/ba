add_executable(app
    "src/main.c"
)

set_target_properties(app PROPERTIES
    OUTPUT_NAME "ba"
    EXPORT_NAME "APP"
)

target_link_libraries(app PRIVATE BA::BA)

add_executable(BA::APP ALIAS app)
