#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/reader.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#ifdef _WIN32
#define fseeko _fseeki64
#endif

struct ba_reader {
  FILE *fp;
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
  if (rd == NULL) {
    errno = EINVAL;
    return;
  }

  if ((*rd)->fp != NULL)
    fclose((*rd)->fp);

  if ((*rd)->entry_headers != NULL)
    free((*rd)->entry_headers);

  if (*rd != NULL)
    free(*rd);

  *rd = NULL;
}

int ba_reader_open(ba_reader_t *rd, const char *filename) {
  if (rd == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  rd->fp = fopen(filename, "rb");
  if (rd->fp == NULL)
    return -1;

  if (fread(&rd->header, sizeof(rd->header), 1, rd->fp) < 1)
    return -1;

  if (rd->header.signature != BA_SIGNATURE) {
    errno = EINVAL;
    return -1;
  }

  rd->entry_headers = calloc(rd->header.entry_size, sizeof(*rd->entry_headers));
  if (rd->entry_headers == NULL)
    return -1;

  if (fread(rd->entry_headers, sizeof(*rd->entry_headers),
            rd->header.entry_size, rd->fp) < rd->header.entry_size)
    return -1;

  return 0;
}

uint64_t ba_reader_size(const ba_reader_t *rd, const char *entry) {
  if (rd == NULL || entry == NULL) {
    errno = EINVAL;
    return 0;
  }

  uint64_t key = ba_hash(entry);
  uint32_t index;

  for (index = 0; index < rd->header.entry_size; index++)
    if (rd->entry_headers[index].key == key)
      break;

  if (index >= rd->header.entry_size || rd->entry_headers[index].key != key) {
    errno = ENOENT;
    return 0;
  }

  return rd->entry_headers[index].o_size;
}

int ba_reader_read(ba_reader_t *rd, const char *entry, void *ptr) {
  if (rd == NULL || entry == NULL || ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  uint64_t key = ba_hash(entry);
  uint32_t index;

  for (index = 0; index < rd->header.entry_size; index++)
    if (rd->entry_headers[index].key == key)
      break;

  if (index >= rd->header.entry_size || rd->entry_headers[index].key != key) {
    errno = ENOENT;
    return -1;
  }

  struct ba_entry_header header = rd->entry_headers[index];

  void *buffer = malloc(header.c_size);
  if (buffer == NULL)
    return -1;

  if (fseeko(rd->fp, header.offset, SEEK_SET) < 0) {
    free(buffer);
    return -1;
  }

  if (fread(buffer, 1, header.c_size, rd->fp) < header.c_size) {
    free(buffer);
    return -1;
  }

  z_stream strm = {0};
  if (inflateInit(&strm) != Z_OK) {
    free(buffer);
    return -1;
  }

  strm.next_in = buffer;
  strm.avail_in = header.c_size;
  strm.next_out = ptr;
  strm.avail_out = header.o_size;

  do {
    int ret = inflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_END)
      break;
    else if (ret != Z_OK) {
      errno = EIO;
      inflateEnd(&strm);
      free(buffer);
      return -1;
    }
  } while (1);

  inflateEnd(&strm);

  free(buffer);

  return 0;
}
