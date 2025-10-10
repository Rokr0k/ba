#ifndef BA_SOURCE_H
#define BA_SOURCE_H

#include "exports.h"
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ba_source {
  /**
   * Free all resources.
   */
  void (*free)(void *arg);
  /**
   * Negative offset means from the end of the source.
   *
   * Returns 0 if succeeded, -1 if failed.
   */
  int (*seek)(void *arg, uint64_t offset);
  /**
   * Read bytes from the source.
   *
   * Returns size of bytes actually read, 0 if EOF or failed.
   */
  uint64_t (*read)(void *arg, void *ptr, uint64_t size);

  /**
   * Pointer to the resources you will access.
   */
  void *arg;
} ba_source_t;

BA_API int ba_source_init_file(ba_source_t *src, const char *filename);

BA_API void ba_source_init_fp(ba_source_t *src, FILE *fp);

BA_API int ba_source_init_mem(ba_source_t *src, void *ptr, uint64_t size,
                              void (*dealloc)(void *));

BA_API int ba_source_init_constmem(ba_source_t *src, const void *ptr,
                                   uint64_t size);

BA_API void ba_source_free(ba_source_t *src);

BA_API int ba_source_seek(ba_source_t *src, uint64_t offset);

BA_API size_t ba_source_read(ba_source_t *src, void *ptr, uint64_t size);

#ifdef __cplusplus
}
#endif

#endif
