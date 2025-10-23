#include "headers.h"
#include "signature.h"
#include <ba/writer.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

struct ba_entry_column {
  char *name;
  uint64_t nlen;
  ba_buffer_t *buf;
};

struct ba_writer {
  uint32_t entry_size;
  uint32_t entry_cap;
  struct ba_entry_column *entries;
};

int ba_writer_alloc(ba_writer_t **wr) {
  if (wr == NULL) {
    errno = EINVAL;
    return -1;
  }

  *wr = calloc(1, sizeof(**wr));
  if (*wr == NULL)
    return -1;

  (*wr)->entry_size = 0;
  (*wr)->entry_cap = 1;
  (*wr)->entries = calloc((*wr)->entry_cap, sizeof(*(*wr)->entries));
  if ((*wr)->entries == NULL) {
    free(*wr);
    *wr = NULL;
    return -1;
  }

  return 0;
}

void ba_writer_free(ba_writer_t **wr) {
  if (wr == NULL || *wr == NULL) {
    errno = EINVAL;
    return;
  }

  for (uint32_t i = 0; i < (*wr)->entry_size; i++) {
    free((*wr)->entries[i].name);
    ba_buffer_free(&(*wr)->entries[i].buf);
  }

  free((*wr)->entries);

  free(*wr);
  *wr = NULL;
}

int ba_writer_add(ba_writer_t *wr, const char *entry, uint64_t entry_len,
                  ba_buffer_t *buf) {
  if (wr == NULL || buf == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (entry_len == 0)
    entry_len = strlen(entry);

  if (wr->entry_size >= wr->entry_cap) {
    uint32_t new_cap = wr->entry_cap << 1;
    struct ba_entry_column *new_entries =
        realloc(wr->entries, new_cap * sizeof(*wr->entries));
    if (new_entries == NULL)
      return -1;
    wr->entries = new_entries;
    wr->entry_cap = new_cap;
  }

  struct ba_entry_column col;

  col.name = malloc(entry_len);
  if (col.name == NULL)
    return -1;
  strncpy(col.name, entry, col.nlen = entry_len);
  col.buf = buf;

  wr->entries[wr->entry_size++] = col;

  return 0;
}

int ba_writer_add_file(ba_writer_t *wr, const char *filename) {
  if (wr == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  ba_buffer_t *buf;
  if (ba_buffer_init_file(&buf, filename, "rb") < 0)
    return -1;

  if (ba_writer_add(wr, filename, 0, buf) < 0) {
    ba_buffer_free(&buf);
    return -1;
  }

  return 0;
}

int ba_writer_write(ba_writer_t *wr, ba_buffer_t *buf) {
  if (wr == NULL || buf == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (ba_buffer_seek(buf,
                     sizeof(struct ba_archive_header) +
                         wr->entry_size * sizeof(struct ba_entry_header),
                     SEEK_SET) < 0)
    return -1;

  struct ba_archive_header header = {0};
  header.sign = BA_SIGNATURE;
  header.ensz = wr->entry_size;

  struct ba_entry_header *entry_headers =
      calloc(wr->entry_size, sizeof(*entry_headers));
  if (entry_headers == NULL)
    return -1;

  for (uint32_t i = 0; i < wr->entry_size; i++) {
    if (ba_buffer_write(buf, wr->entries[i].name, wr->entries[i].nlen) < 0) {
      free(entry_headers);
      return -1;
    }

    entry_headers[i].tidx = header.tbsz;
    entry_headers[i].tlen = wr->entries[i].nlen;

    header.tbsz += entry_headers[i].tlen;
  }

  z_stream strm = {0};

  for (uint32_t i = 0; i < wr->entry_size; i++) {
    int64_t size = ba_buffer_size(wr->entries[i].buf);
    if (size == -1LL) {
      free(entry_headers);
      return -1;
    }

    if (ba_buffer_seek(wr->entries[i].buf, 0, SEEK_SET) < 0) {
      free(entry_headers);
      return -1;
    }
    void *data = malloc(size);
    if (data == NULL) {
      free(entry_headers);
      return -1;
    }
    if (ba_buffer_read(wr->entries[i].buf, data, size) < size) {
      free(data);
      free(entry_headers);
      return -1;
    }

    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
      free(data);
      free(entry_headers);
      return -1;
    }

    uint64_t size_in = size;
    void *buffer_in = data;
    uint64_t size_out = deflateBound(&strm, size_in);
    void *buffer_out = malloc(size_out);
    if (buffer_out == NULL) {
      deflateEnd(&strm);
      free(data);
      free(entry_headers);
      return -1;
    }

    strm.next_in = buffer_in;
    strm.avail_in = size_in;
    strm.next_out = buffer_out;
    strm.avail_out = size_out;

    do {
      int ret = deflate(&strm, Z_FINISH);
      if (ret == Z_STREAM_END)
        break;
      else if (ret != Z_OK) {
        errno = EIO;
        deflateEnd(&strm);
        free(buffer_out);
        free(data);
        free(entry_headers);
        return -1;
      }
    } while (1);

    entry_headers[i].boff = ba_buffer_tell(buf);
    entry_headers[i].bosz = strm.total_in;
    entry_headers[i].bcsz = strm.total_out;

    if (ba_buffer_write(buf, buffer_out, entry_headers[i].bcsz) < 0) {
      deflateEnd(&strm);
      free(buffer_out);
      free(data);
      free(entry_headers);
      return -1;
    }

    deflateEnd(&strm);
    free(buffer_out);
    free(data);
  }

  if (ba_buffer_seek(buf, 0, SEEK_SET) < 0) {
    free(entry_headers);
    return -1;
  }

  if (ba_buffer_write(buf, &header, sizeof(header)) < 0) {
    free(entry_headers);
    return -1;
  }

  if (ba_buffer_write(buf, entry_headers,
                      wr->entry_size * sizeof(*entry_headers)) < 0) {
    free(entry_headers);
    return -1;
  }

  free(entry_headers);

  return 0;
}

int ba_writer_write_file(ba_writer_t *wr, const char *filename) {
  if (wr == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  ba_buffer_t *buf;
  if (ba_buffer_init_file(&buf, filename, "wb") < 0)
    return -1;

  if (ba_writer_write(wr, buf) < 0) {
    ba_buffer_free(&buf);
    return -1;
  }

  ba_buffer_free(&buf);

  return 0;
}
