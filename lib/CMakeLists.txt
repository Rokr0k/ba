add_library(ba
    "src/archive.c"
    "src/source.c"
    "src/hash.c"
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
    "include/ba/archive.h"
    "include/ba/source.h"
)

target_compile_definitions(ba PRIVATE _FILE_OFFSET_BITS=64)

add_library(BA::BA ALIAS ba)
