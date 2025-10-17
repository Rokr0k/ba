#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/reader.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

struct ba_reader {
  ba_source_t src;
  struct ba_archive_header header;
  struct ba_entry_header *entry_headers;
};

int ba_reader_alloc(ba_reader_t **rd) {
  if (rd == NULL) {
    errno = EINVAL;
    return -1;
  }

  *rd = calloc(1, sizeof(**rd));
  if (*rd == NULL)
    return -1;

  return 0;
}

void ba_reader_free(ba_reader_t **rd) {
  if (rd == NULL || *rd == NULL) {
    errno = EINVAL;
    return;
  }

  ba_source_free(&(*rd)->src);

  free((*rd)->entry_headers);

  free(*rd);
  *rd = NULL;
}

int ba_reader_open(ba_reader_t *rd, ba_source_t *src) {
  if (rd == NULL || src == NULL) {
    errno = EINVAL;
    return -1;
  }

  rd->src = *src;

  ba_source_init(src);

  if (ba_source_seek(&rd->src, 0, SEEK_SET) == -1)
    return -1;

  if (ba_source_read(&rd->src, &rd->header, sizeof(rd->header)) <
      sizeof(rd->header)) {
    errno = ENOMSG;
    return -1;
  }

  if (rd->header.signature != BA_SIGNATURE)
    return -1;

  rd->entry_headers = calloc(rd->header.entry_size, sizeof(*rd->entry_headers));
  if (rd->entry_headers == NULL)
    return -1;

  if (ba_source_read(&rd->src, rd->entry_headers,
                     rd->header.entry_size * sizeof(*rd->entry_headers)) <
      rd->header.entry_size * sizeof(*rd->entry_headers)) {
    errno = ENOMSG;
    return -1;
  }

  return 0;
}

int ba_reader_read(ba_reader_t *rd, const char *entry, void **ptr,
                   size_t *size) {
  if (rd == NULL || entry == NULL || ptr == NULL || size == NULL) {
    errno = EINVAL;
    return -1;
  }

  *ptr = NULL;
  *size = 0;

  uint64_t key = ba_hash(entry);
  uint32_t index;

  for (index = 0; index < rd->header.entry_size; index++)
    if (key == rd->entry_headers[index].key)
      break;

  if (index >= rd->header.entry_size || key != rd->entry_headers[index].key) {
    errno = ENOENT;
    return -1;
  }

  struct ba_entry_header header = rd->entry_headers[index];

  if (ba_source_seek(&rd->src, header.offset, SEEK_SET) < 0)
    return -1;

  uint8_t *in_buffer = malloc(header.e_size);
  if (in_buffer == NULL)
    return -1;
  uint8_t *out_buffer = malloc(header.o_size);
  if (out_buffer == NULL) {
    free(in_buffer);
    return -1;
  }

  uint64_t buffer_size = ba_source_read(&rd->src, in_buffer, header.e_size);
  if (buffer_size < header.e_size) {
    free(in_buffer);
    free(out_buffer);
    return -1;
  }

  z_stream strm = {0};
  strm.next_in = in_buffer;
  strm.avail_in = buffer_size;
  strm.next_out = out_buffer;
  strm.avail_out = header.o_size;
  switch (inflateInit(&strm)) {
  case Z_MEM_ERROR:
    errno = ENOMEM;
    free(in_buffer);
    free(out_buffer);
    return -1;

  case Z_STREAM_ERROR:
    errno = EINVAL;
    free(in_buffer);
    free(out_buffer);
    return -1;

  case Z_VERSION_ERROR:
    errno = EBADR;
    free(in_buffer);
    free(out_buffer);
    return -1;
  }

  inflate(&strm, Z_FINISH);

  *ptr = out_buffer;
  *size = strm.total_out;

  inflateEnd(&strm);
  free(in_buffer);

  return 0;
}
