#ifndef BA_READER_H
#define BA_READER_H

#include "buffer.h"
#include "exports.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_reader ba_reader_t;

typedef uint32_t ba_id_t;

#define BA_ENTRY_INVALID ((ba_id_t)~0)

BA_API int ba_reader_alloc(ba_reader_t **rd);
BA_API void ba_reader_free(ba_reader_t **rd);

BA_API int ba_reader_open(ba_reader_t *rd, ba_buffer_t *buf);
BA_API int ba_reader_open_file(ba_reader_t *rd, const char *filename);

BA_API uint32_t ba_reader_size(const ba_reader_t *rd);

BA_API ba_id_t ba_reader_find_entry(const ba_reader_t *rd, const char *entry,
                                    uint64_t entry_len);

BA_API int ba_reader_entry_name(const ba_reader_t *rd, ba_id_t id,
                                const char **str, uint64_t *len);

BA_API uint64_t ba_reader_entry_size(const ba_reader_t *rd, ba_id_t id);

BA_API int ba_reader_read(ba_reader_t *rd, ba_id_t id, void *ptr);

#ifdef __cplusplus
}
#endif

#endif
