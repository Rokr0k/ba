configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

add_library(ba
    "src/ba.c"
    "src/buffer.c"
    "src/reader.c"
    "src/writer.c"
)

set_target_properties(ba
    PROPERTIES
    OUTPUT_NAME "ba"
    EXPORT_NAME "BA"
)

target_sources(ba
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "include"
    FILES
    "include/ba/ba.h"
    "include/ba/exports.h"
    "include/ba/buffer.h"
    "include/ba/reader.h"
    "include/ba/writer.h"
)

target_include_directories(ba
    PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_definitions(ba
    PRIVATE
    _FILE_OFFSET_BITS=64
)

target_link_libraries(ba
    PRIVATE
    ZLIB::ZLIB
)

add_library(BA::BA ALIAS ba)
