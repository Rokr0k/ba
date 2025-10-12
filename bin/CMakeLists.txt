configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

add_executable(app
    "src/main.c"
)

target_sources(app
    PRIVATE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

set_target_properties(app PROPERTIES
    OUTPUT_NAME "ba"
    EXPORT_NAME "APP"
)

target_link_libraries(app PRIVATE BA::BA)

add_executable(BA::APP ALIAS app)
