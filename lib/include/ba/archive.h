#ifndef BA_ARCHIVE_H
#define BA_ARCHIVE_H

#include "exports.h"
#include "source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_archive ba_archive_t;

BA_API int ba_archive_alloc(ba_archive_t **ar);
BA_API void ba_archive_free(ba_archive_t **ar);

BA_API int ba_archive_open(ba_archive_t *ar, const ba_source_t *src);

BA_API int ba_archive_read(ba_archive_t *ar, const char *entry, void **ptr,
                           uint64_t *size);

#ifdef __cplusplus
}
#endif

#endif
