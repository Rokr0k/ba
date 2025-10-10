add_library(ba
    "src/archive.c"
    "src/source.c"
    "src/hash.c"
    "src/tree.c"
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

if(ZLIB_FOUND)

    target_link_libraries(ba PRIVATE ZLIB::ZLIB)

    target_compile_definitions(ba PRIVATE BA_FOUND_ZLIB)

endif(ZLIB_FOUND)

add_library(BA::BA ALIAS ba)
