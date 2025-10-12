#ifndef BA_READER_H
#define BA_READER_H

#include "exports.h"
#include "source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_reader ba_reader_t;

BA_API int ba_reader_alloc(ba_reader_t **rd);
BA_API void ba_reader_free(ba_reader_t **rd);

BA_API int ba_reader_open(ba_reader_t *rd, ba_source_t *src);

BA_API int ba_reader_read(ba_reader_t *rd, const char *entry, void **ptr,
                          uint64_t *size);

#ifdef __cplusplus
}
#endif

#endif
