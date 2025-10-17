#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/writer.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <zlib.h>

struct ba_entry_column {
  uint64_t key;
  char *filename;
};

struct ba_writer {
  struct ba_archive_header header;
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

  (*wr)->header.signature = BA_SIGNATURE;
  (*wr)->header.entry_size = 0;
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

  for (uint32_t i = 0; i < (*wr)->header.entry_size; i++)
    free((*wr)->entries[i].filename);

  free((*wr)->entries);

  free(*wr);
  *wr = NULL;
}

int ba_writer_add(ba_writer_t *wr, const char *filename) {
  if (wr == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (wr->header.entry_size >= wr->entry_cap) {
    uint32_t new_cap = wr->entry_cap << 1;
    struct ba_entry_column *new_entries =
        realloc(wr->entries, new_cap * sizeof(*wr->entries));
    if (new_entries == NULL)
      return -1;

    wr->entries = new_entries;
    wr->entry_cap = new_cap;
  }

  wr->entries[wr->header.entry_size].key = ba_hash(filename);
  wr->entries[wr->header.entry_size].filename = strdup(filename);
  if (wr->entries[wr->header.entry_size].filename == NULL)
    return -1;

  wr->header.entry_size++;

  return 0;
}

int ba_writer_write(ba_writer_t *wr, ba_source_t *src) {
  if (wr == NULL || src == NULL) {
    errno = EINVAL;
    return -1;
  }

  uint64_t content_offset =
      sizeof(wr->header) +
      wr->header.entry_size * sizeof(struct ba_entry_header);

  if (ba_source_seek(src, content_offset, SEEK_SET) < 0)
    return -1;

  struct ba_entry_header *entries =
      calloc(wr->header.entry_size, sizeof(*entries));
  if (entries == NULL)
    return -1;

  z_stream strm = {0};
  uint32_t errors = 0;
  for (uint32_t i = 0; i < wr->header.entry_size; i++) {
    int fd = open(wr->entries[i + errors].filename, O_RDONLY);
    if (fd == -1) {
      wr->header.entry_size--;
      i--;
      errors++;
      continue;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
      wr->header.entry_size--;
      i--;
      errors++;
      close(fd);
      continue;
    }

    size_t size_in = st.st_size;
    void *buf_in = mmap(NULL, size_in, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf_in == MAP_FAILED) {
      wr->header.entry_size--;
      i--;
      errors++;
      close(fd);
      continue;
    }

    close(fd);

    switch (deflateInit(&strm, Z_DEFAULT_COMPRESSION)) {
    case Z_MEM_ERROR:
      free(entries);
      munmap(buf_in, size_in);
      errno = ENOMEM;
      return -1;

    case Z_STREAM_ERROR:
      free(entries);
      munmap(buf_in, size_in);
      errno = EINVAL;
      return -1;

    case Z_VERSION_ERROR:
      free(entries);
      munmap(buf_in, size_in);
      errno = EBADR;
      return -1;
    }

    size_t size_out = deflateBound(&strm, size_in);
    void *buf_out = malloc(size_out);
    if (buf_out == NULL) {
      free(entries);
      munmap(buf_in, size_in);
      deflateEnd(&strm);
      return -1;
    }

    strm.next_in = buf_in;
    strm.avail_in = size_in;
    strm.next_out = buf_out;
    strm.avail_out = size_out;

    deflate(&strm, Z_FINISH);

    if (ba_source_write(src, buf_out, strm.total_out) < 0) {
      free(entries);
      munmap(buf_in, size_in);
      free(buf_out);
      deflateEnd(&strm);
      return -1;
    }

    deflateEnd(&strm);

    munmap(buf_in, size_in);
    free(buf_out);

    entries[i].key = wr->entries[i + errors].key;
    entries[i].offset = content_offset;
    entries[i].e_size = strm.total_out;
    entries[i].o_size = strm.total_in;

    content_offset += strm.total_out;
  }

  if (ba_source_seek(src, 0, SEEK_SET) < 0) {
    free(entries);
    return -1;
  }

  if (ba_source_write(src, &wr->header, sizeof(wr->header)) < 0) {
    free(entries);
    return -1;
  }

  if (ba_source_write(src, entries, wr->header.entry_size * sizeof(*entries)) <
      0) {
    free(entries);
    return -1;
  }

  free(entries);

  return 0;
}
