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
   * Seek to certain point of the source.
   *
   * Seeking beyond source size is allowed.
   *
   * Returns `0` if succeeded, `-1` if failed.
   */
  int (*seek)(void *arg, int64_t offset, int whence);
  /**
   * Get current point of the source.
   *
   * Returns point position if succeeded, `-1` if failed.
   */
  int64_t (*tell)(void *arg);
  /**
   * Read bytes from the source.
   *
   * Returns size of bytes actually read, `0` if `EOF` or failed.
   */
  uint64_t (*read)(void *arg, void *ptr, uint64_t size);
  /**
   * Write byte into the source.
   *
   * Returns `0` if succeeded, `-1` if failed.
   */
  int (*write)(void *arg, const void *ptr, uint64_t size);

  /**
   * Pointer to the resources you will access.
   */
  void *arg;
} ba_source_t;

BA_API void ba_source_init(ba_source_t *src);

BA_API int ba_source_init_file(ba_source_t *src, const char *filename);

BA_API void ba_source_init_fp(ba_source_t *src, FILE *fp);

BA_API int ba_source_init_mem(ba_source_t *src, const void *ptr, uint64_t size);

BA_API void ba_source_free(ba_source_t *src);

BA_API int ba_source_seek(ba_source_t *src, int64_t offset, int whence);

BA_API int64_t ba_source_tell(ba_source_t *src);

BA_API uint64_t ba_source_read(ba_source_t *src, void *ptr, uint64_t size);

BA_API int ba_source_write(ba_source_t *src, const void *ptr, uint64_t size);

#ifdef __cplusplus
}
#endif

#endif
