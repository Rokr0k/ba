add_library(ba
    "src/source.c"
    "src/hash.c"
    "src/reader.c"
    "src/writer.c"
)

set_target_properties(ba PROPERTIES
    OUTPUT_NAME "ba"
    EXPORT_NAME "BA"
)

target_sources(ba
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "include"
    FILES
    "include/ba/exports.h"
    "include/ba/source.h"
    "include/ba/reader.h"
    "include/ba/writer.h"
)

target_compile_definitions(ba PRIVATE _FILE_OFFSET_BITS=64)

add_library(BA::BA ALIAS ba)
