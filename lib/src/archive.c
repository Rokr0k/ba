#include "hash.h"
#include "signature.h"
#include <ba/archive.h>
#include <stdint.h>
#include <stdlib.h>

struct ba_archive_header {
  uint32_t signature;
  uint32_t entry_size;
};

struct ba_entry_header {
  uint64_t key;
  uint64_t offset;
  uint64_t size;
};

struct ba_archive {
  ba_source_t src;
  struct ba_archive_header header;
  struct ba_entry_header *entry_headers;
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

  free((*ar)->entry_headers);

  free(*ar);
  *ar = NULL;
}

int ba_archive_open(ba_archive_t *ar, const ba_source_t *src) {
  if (ar == NULL || src == NULL)
    return -1;

  ar->src = *src;

  if (ba_source_seek(&ar->src, 0, SEEK_SET) == -1)
    return -1;

  if (ba_source_read(&ar->src, &ar->header, sizeof(ar->header)) <
      sizeof(ar->header))
    return -1;

  if (ar->header.signature != BA_SIGNATURE)
    return -1;

  ar->entry_headers = calloc(ar->header.entry_size, sizeof(*ar->entry_headers));
  if (ar->entry_headers == NULL)
    return -1;

  if (ba_source_read(&ar->src, ar->entry_headers,
                     ar->header.entry_size * sizeof(*ar->entry_headers)) <
      ar->header.entry_size * sizeof(*ar->entry_headers))
    return -1;

  return 0;
}

int ba_archive_read(ba_archive_t *ar, const char *entry, void **ptr,
                    size_t *size) {
  if (ar == NULL || entry == NULL || ptr == NULL || size == NULL)
    return -1;

  uint64_t key = ba_hash(entry);
  uint32_t index;

  for (index = 0; index < ar->header.entry_size; index++)
    if (key == ar->entry_headers[index].key)
      break;

  if (index >= ar->header.entry_size || key == ar->entry_headers[index].key)
    return -1;

  struct ba_entry_header header = ar->entry_headers[index];

  if (ba_source_seek(&ar->src, header.offset, SEEK_SET) < 0)
    return -1;

  *ptr = malloc(header.size);
  if (*ptr == NULL)
    return -1;
  *size = header.size;

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
