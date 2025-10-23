#ifndef BA_BUFFER_H
#define BA_BUFFER_H

#include "exports.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_buffer ba_buffer_t;

BA_API int ba_buffer_init(ba_buffer_t **buf);

BA_API int ba_buffer_init_mem(ba_buffer_t **buf, const void *ptr,
                              uint64_t size);

BA_API int ba_buffer_init_file(ba_buffer_t **buf, const char *filename,
                               const char *mode);

BA_API void ba_buffer_free(ba_buffer_t **buf);

BA_API int ba_buffer_seek(ba_buffer_t *buf, int64_t pos, int whence);

BA_API int64_t ba_buffer_tell(ba_buffer_t *buf);

BA_API uint64_t ba_buffer_read(ba_buffer_t *buf, void *ptr, uint64_t size);

BA_API int ba_buffer_write(ba_buffer_t *buf, const void *ptr, uint64_t size);

BA_API uint64_t ba_buffer_size(ba_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif
