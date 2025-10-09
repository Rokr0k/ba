#include <ba/archive.h>
#include <stdint.h>
#include <stdlib.h>

struct ba_archive_header {
  uint32_t ent_sz;
  uint8_t flgs;
};

struct ba_entry_header {
  uint64_t offset;
  uint64_t size;
};

struct ba_archive {
  ba_source_t src;
  struct ba_archive_header hd;
  struct ba_entry_header *ent;
};

int ba_archive_alloc(ba_archive_t **ar) {
  if (ar == NULL)
    return -1;

  *ar = malloc(sizeof(**ar));
  if (*ar == NULL)
    return -1;

  ba_source_init(&(*ar)->src);

  (*ar)->ent = NULL;

  return 0;
}

void ba_archive_free(ba_archive_t **ar) {
  if (ar == NULL || *ar == NULL)
    return;

  ba_source_free(&(*ar)->src);

  free((*ar)->ent);

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

  ar->ent = calloc(ar->hd.ent_sz, sizeof(*ar->ent));
  if (ar->ent == NULL)
    return -1;

  return 0;
}
