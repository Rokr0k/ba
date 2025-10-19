#ifndef BA_READER_H
#define BA_READER_H

#include "exports.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_reader ba_reader_t;

BA_API int ba_reader_alloc(ba_reader_t **rd);
BA_API void ba_reader_free(ba_reader_t **rd);

BA_API int ba_reader_open(ba_reader_t *rd, const char *filename);

BA_API uint64_t ba_reader_size(const ba_reader_t *rd, const char *entry);

BA_API int ba_reader_read(ba_reader_t *rd, const char *entry, void *ptr);

#ifdef __cplusplus
}
#endif

#endif
