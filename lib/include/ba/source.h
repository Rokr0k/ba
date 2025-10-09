#ifndef BA_SOURCE_H
#define BA_SOURCE_H

#include "exports.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int ba_source_type_t;

#define BA_SOURCE_UNK 0
#define BA_SOURCE_FP 1
#define BA_SOURCE_MEM 2
#define BA_SOURCE_CONSTMEM 3

struct ba_source_fp {
  ba_source_type_t type;
  FILE *fp;
};

struct ba_source_constmem {
  ba_source_type_t type;
  const void *ptr;
  size_t size;
  size_t cursor;
};

struct ba_source_mem {
  ba_source_type_t type;
  void *ptr;
  size_t size;
  size_t cursor;
  void (*mesiah)(void *);
};

typedef union ba_source {
  ba_source_type_t type;
  struct ba_source_fp fp;
  struct ba_source_mem mem;
  struct ba_source_constmem constmem;
} ba_source_t;

/**
 * Initialize the source object with default values.
 *
 * Source object initialized with this function is invalid for any operations.
 *
 * Initializing an already initialized object might leak resources.
 */
BA_API void ba_source_init(ba_source_t *src);

/**
 * Initialize the source object with file pointer in `stdio.h`.
 *
 * The source object owns the file pointer afterwards.
 *
 * Initializing an already initialized object might leak resources.
 */
BA_API void ba_source_init_fp(ba_source_t *src, FILE *fp);

/**
 * Initialize the source object with memory.
 *
 * Operations might cause segfault if you give wrong pointer and/or size.
 *
 * The source object owns the memory afterwards.
 * If `mesiah` is NULL, it uses `free` from `stdlib.h`.
 *
 * Initializing an already initialized object might leak resources.
 */
BA_API void ba_source_init_mem(ba_source_t *src, void *ptr, size_t size,
                               void (*mesiah)(void *));

/**
 * Initialize the source object with memory.
 *
 * Operations might cause segfault if you give wrong pointer and/or size.
 * The memory destination has to be kept alive while you use the source object.
 *
 * Initializing an already initialized object might leak resources.
 */
BA_API void ba_source_init_constmem(ba_source_t *src, const void *ptr,
                                    size_t size);

/**
 * Release resources used for source object.
 */
BA_API void ba_source_free(ba_source_t *src);

/**
 * Seek to certain offset in source.
 *
 * Negative value means from end of the source.
 *
 * Returns new position if succeeded, `(size_t)-1` if failed.
 */
BA_API size_t ba_source_seek(ba_source_t *src, off_t offset);

/**
 * Get current position in source.
 *
 * Returns current position if succeeded, `(size_t)-1` if failed.
 */
BA_API size_t ba_source_tell(ba_source_t *src);

/**
 * Read memory from source.
 *
 * Might cause segfault if you give wrong `ptr`, or the allocated size is less
 * than `size`.
 *
 * Returns number of bytes actually read, `0` if EOF, `(size_t)-1` if failed.
 */
BA_API size_t ba_source_read(ba_source_t *src, void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
