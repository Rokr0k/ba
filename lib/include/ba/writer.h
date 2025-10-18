#ifndef BA_WRITER_H
#define BA_WRITER_H

#include "exports.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_writer ba_writer_t;

BA_API int ba_writer_alloc(ba_writer_t **wr);
BA_API void ba_writer_free(ba_writer_t **wr);

BA_API int ba_writer_add(ba_writer_t *wr, const char *filename);

BA_API int ba_writer_write(ba_writer_t *wr, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
