#include "hash.h"
#include "tree.h"
#include <ba/archive.h>
#include <stdint.h>
#include <stdlib.h>

struct ba_archive_header {
  uint32_t entry_size;
  uint8_t compression;
  uint8_t node_size;
  uint16_t depth;
  uint64_t root;
};

struct ba_entry_header {
  uint64_t offset;
  uint64_t size;
};

struct ba_archive {
  ba_source_t src;
  struct ba_archive_header hd;
};

int ba_archive_alloc(ba_archive_t **ar) {
  if (ar == NULL)
    return -1;

  *ar = calloc(1, sizeof(**ar));
  if (*ar == NULL)
    return -1;

  return 0;
}

void ba_archive_free(ba_archive_t **ar) {
  if (ar == NULL || *ar == NULL)
    return;

  ba_source_free(&(*ar)->src);

  free(*ar);
  *ar = NULL;
}

int ba_archive_open(ba_archive_t *ar, const ba_source_t *src) {
  if (ar == NULL || src == NULL)
    return -1;

  ar->src = *src;

  if (ba_source_seek(&ar->src, 0) == -1)
    return -1;

  if (ba_source_read(&ar->src, &ar->hd, sizeof(ar->hd)) < sizeof(ar->hd))
    return -1;

  return 0;
}

int ba_archive_read(ba_archive_t *ar, const char *entry, void **ptr,
                    size_t *size) {
  if (ar == NULL || entry == NULL || ptr == NULL || size == NULL)
    return -1;

  uint64_t query = ba_hash(entry);
  uint64_t p;
  if (ba_tree_find(&ar->src, ar->hd.root, ar->hd.node_size, ar->hd.depth, query,
                   &p) < 0)
    return -1;

  if (ba_source_seek(&ar->src, p) < 0)
    return -1;

  struct ba_entry_header hd;
  if (ba_source_read(&ar->src, &hd, sizeof(hd)) < 0)
    return -1;

  if (ba_source_seek(&ar->src, hd.offset) < 0)
    return -1;

  *ptr = malloc(hd.size);
  if (*ptr == NULL)
    return -1;
  *size = hd.size;

  uint64_t read;
  if ((read = ba_source_read(&ar->src, *ptr, *size)) == 0) {
    free(*ptr);
    *ptr = NULL;
    *size = 0;
    return -1;
  }

  *size = read;

  return 0;
}
