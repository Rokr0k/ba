#include "headers.h"
#include "signature.h"
#include <ba/reader.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

struct ba_reader {
  void *data;
  const struct ba_archive_header *ahdr;
  const struct ba_entry_header *ehdr;
  const char *tble;
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

  free((*rd)->data);

  free(*rd);

  *rd = NULL;
}

int ba_reader_open(ba_reader_t *rd, const void *ptr, uint64_t size) {
  if (rd == NULL || ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  rd->data = malloc(size);
  if (rd->data == NULL)
    return -1;

  memcpy(rd->data, ptr, size);

  rd->ahdr = rd->data;
  rd->ehdr = (const struct ba_entry_header *)&rd->ahdr[1];
  rd->tble = (const char *)&rd->ehdr[rd->ahdr->ensz];

  if (rd->ahdr->sign != BA_SIGNATURE) {
    errno = EINVAL;
    return -1;
  }

  return 0;
}

int ba_reader_open_file(ba_reader_t *rd, const char *filename) {
  if (rd == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL)
    return -1;

  if (fseeko(fp, 0, SEEK_END) < 0) {
    fclose(fp);
    return -1;
  }

  int64_t size = ftello(fp);
  if (size < 0) {
    fclose(fp);
    return -1;
  }

  if (fseeko(fp, 0, SEEK_SET) < 0) {
    fclose(fp);
    return -1;
  }

  void *data = malloc(size);
  if (data == NULL) {
    fclose(fp);
    return -1;
  }

  if (fread(data, 1, size, fp) < size) {
    free(data);
    fclose(fp);
    return -1;
  }

  fclose(fp);

  int ret = ba_reader_open(rd, data, size);

  free(data);

  return ret;
}

uint32_t ba_reader_size(const ba_reader_t *rd) {
  if (rd == NULL) {
    errno = EINVAL;
    return 0;
  }

  return rd->ahdr->ensz;
}

ba_id_t ba_reader_find_entry(const ba_reader_t *rd, const char *entry,
                             uint64_t entry_len) {
  if (rd == NULL || entry == NULL) {
    errno = EINVAL;
    return BA_ENTRY_INVALID;
  }

  if (entry_len == 0)
    entry_len = strlen(entry);

  ba_id_t id;
  for (id = 0; id < rd->ahdr->ensz; id++)
    if (entry_len == rd->ehdr[id].tlen &&
        strncmp(entry, &rd->tble[rd->ehdr[id].tidx], rd->ehdr[id].tlen) == 0)
      break;
  if (id >= rd->ahdr->ensz) {
    errno = ENOENT;
    return BA_ENTRY_INVALID;
  }

  return id;
}

int ba_reader_entry_name(const ba_reader_t *rd, ba_id_t id, const char **str,
                         uint64_t *len) {
  if (rd == NULL || id >= rd->ahdr->ensz || str == NULL || len == NULL) {
    errno = EINVAL;
    return -1;
  }

  *str = &rd->tble[rd->ehdr[id].tidx];
  *len = rd->ehdr[id].tlen;

  return 0;
}

uint64_t ba_reader_entry_size(const ba_reader_t *rd, ba_id_t id) {
  if (rd == NULL || id >= rd->ahdr->ensz) {
    errno = EINVAL;
    return 0;
  }

  return rd->ehdr[id].bosz;
}

int ba_reader_read(ba_reader_t *rd, ba_id_t id, void *ptr) {
  if (rd == NULL || id >= rd->ahdr->ensz || ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  z_stream strm = {0};
  if (inflateInit(&strm) != Z_OK) {
    errno = EIO;
    return -1;
  }

  strm.next_in = &((uint8_t *)rd->data)[rd->ehdr[id].boff];
  strm.avail_in = rd->ehdr[id].bcsz;
  strm.next_out = ptr;
  strm.avail_out = rd->ehdr[id].bosz;

  do {
    int ret = inflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_END)
      break;
    else if (ret != Z_OK) {
      errno = EIO;
      inflateEnd(&strm);
      return -1;
    }
  } while (1);

  inflateEnd(&strm);

  return 0;
}
