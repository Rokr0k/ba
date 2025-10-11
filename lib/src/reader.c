#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/reader.h>
#include <stdint.h>
#include <stdlib.h>

struct ba_reader {
  ba_source_t src;
  struct ba_archive_header header;
  struct ba_entry_header *entry_headers;
};

int ba_reader_alloc(ba_reader_t **rd) {
  if (rd == NULL)
    return -1;

  *rd = calloc(1, sizeof(**rd));
  if (*rd == NULL)
    return -1;

  return 0;
}

void ba_reader_free(ba_reader_t **rd) {
  if (rd == NULL || *rd == NULL)
    return;

  ba_source_free(&(*rd)->src);

  free((*rd)->entry_headers);

  free(*rd);
  *rd = NULL;
}

int ba_reader_open(ba_reader_t *rd, const ba_source_t *src) {
  if (rd == NULL || src == NULL)
    return -1;

  rd->src = *src;

  if (ba_source_seek(&rd->src, 0, SEEK_SET) == -1)
    return -1;

  if (ba_source_read(&rd->src, &rd->header, sizeof(rd->header)) <
      sizeof(rd->header))
    return -1;

  if (rd->header.signature != BA_SIGNATURE)
    return -1;

  rd->entry_headers = calloc(rd->header.entry_size, sizeof(*rd->entry_headers));
  if (rd->entry_headers == NULL)
    return -1;

  if (ba_source_read(&rd->src, rd->entry_headers,
                     rd->header.entry_size * sizeof(*rd->entry_headers)) <
      rd->header.entry_size * sizeof(*rd->entry_headers))
    return -1;

  return 0;
}

int ba_reader_read(ba_reader_t *rd, const char *entry, void **ptr,
                   size_t *size) {
  if (rd == NULL || entry == NULL || ptr == NULL || size == NULL)
    return -1;

  uint64_t key = ba_hash(entry);
  uint32_t index;

  for (index = 0; index < rd->header.entry_size; index++)
    if (key == rd->entry_headers[index].key)
      break;

  if (index >= rd->header.entry_size || key == rd->entry_headers[index].key)
    return -1;

  struct ba_entry_header header = rd->entry_headers[index];

  if (ba_source_seek(&rd->src, header.offset, SEEK_SET) < 0)
    return -1;

  *ptr = malloc(header.size);
  if (*ptr == NULL)
    return -1;
  *size = header.size;

  uint64_t read;
  if ((read = ba_source_read(&rd->src, *ptr, *size)) == 0) {
    free(*ptr);
    *ptr = NULL;
    *size = 0;
    return -1;
  }

  *size = read;

  return 0;
}
