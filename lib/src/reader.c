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
  ba_buffer_t *buf;
  struct ba_archive_header ahdr;
  struct ba_entry_header *ehdr;
  char *tble;
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

  ba_buffer_free(&(*rd)->buf);
  free((*rd)->ehdr);
  free((*rd)->tble);

  free(*rd);

  *rd = NULL;
}

int ba_reader_open(ba_reader_t *rd, ba_buffer_t *buf) {
  if (rd == NULL || buf == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (ba_buffer_init(&rd->buf) < 0)
    return -1;

  if (ba_buffer_seek(buf, 0, SEEK_SET) < 0)
    return -1;

  char buffer[1 << 16];
  uint64_t read;
  while ((read = ba_buffer_read(buf, buffer, sizeof(buffer))) > 0) {
    if (ba_buffer_write(rd->buf, buffer, read) < 0)
      return -1;
  }

  if (ba_buffer_seek(rd->buf, 0, SEEK_SET) < 0)
    return -1;

  if (ba_buffer_read(rd->buf, &rd->ahdr, sizeof(rd->ahdr)) < sizeof(rd->ahdr))
    return -1;

  if (rd->ahdr.sign != BA_SIGNATURE) {
    errno = EINVAL;
    return -1;
  }

  rd->ehdr = malloc(rd->ahdr.ensz * sizeof(*rd->ehdr));
  if (rd->ehdr == NULL)
    return -1;

  if (ba_buffer_read(rd->buf, rd->ehdr, rd->ahdr.ensz * sizeof(*rd->ehdr)) <
      rd->ahdr.ensz * sizeof(*rd->ehdr))
    return -1;

  rd->tble = malloc(rd->ahdr.tbsz);
  if (rd->tble == NULL)
    return -1;

  if (ba_buffer_read(rd->buf, rd->tble, rd->ahdr.tbsz) < rd->ahdr.tbsz)
    return -1;

  return 0;
}

int ba_reader_open_file(ba_reader_t *rd, const char *filename) {
  if (rd == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  ba_buffer_t *buf;
  if (ba_buffer_init_file(&buf, filename, "rb") < 0)
    return -1;

  if (ba_reader_open(rd, buf) < 0) {
    ba_buffer_free(&buf);
    return -1;
  }

  ba_buffer_free(&buf);

  return 0;
}

uint32_t ba_reader_size(const ba_reader_t *rd) {
  if (rd == NULL) {
    errno = EINVAL;
    return 0;
  }

  return rd->ahdr.ensz;
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
  for (id = 0; id < rd->ahdr.ensz; id++)
    if (entry_len == rd->ehdr[id].tlen &&
        strncmp(entry, &rd->tble[rd->ehdr[id].tidx], rd->ehdr[id].tlen) == 0)
      break;
  if (id >= rd->ahdr.ensz) {
    errno = ENOENT;
    return BA_ENTRY_INVALID;
  }

  return id;
}

int ba_reader_entry_name(const ba_reader_t *rd, ba_id_t id, const char **str,
                         uint64_t *len) {
  if (rd == NULL || str == NULL || len == NULL) {
    errno = EINVAL;
    return -1;
  }

  *str = &rd->tble[rd->ehdr[id].tidx];
  *len = rd->ehdr[id].tlen;

  return 0;
}

uint64_t ba_reader_entry_size(const ba_reader_t *rd, ba_id_t id) {
  if (rd == NULL) {
    errno = EINVAL;
    return 0;
  }

  return rd->ehdr[id].bosz;
}

int ba_reader_read(ba_reader_t *rd, ba_id_t id, void *ptr) {
  if (rd == NULL || ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  void *data = malloc(rd->ehdr[id].bcsz);
  if (data == NULL)
    return -1;
  if (ba_buffer_read(rd->buf, data, rd->ehdr[id].bcsz) < rd->ehdr[id].bcsz) {
    free(data);
    return -1;
  }

  z_stream strm = {0};
  if (inflateInit(&strm) != Z_OK) {
    errno = EIO;
    free(data);
    return -1;
  }

  strm.next_in = data;
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
      free(data);
      return -1;
    }
  } while (1);

  inflateEnd(&strm);
  free(data);

  return 0;
}
