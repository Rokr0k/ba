#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/writer.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

#ifdef _WIN32
#define fseeko _fseeki64
#define ftello _ftelli64
#define strdup _strdup
#endif

struct ba_entry_column {
  uint64_t key;
  void *ptr;
  uint64_t size;
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
    free((*wr)->entries[i].ptr);

  free((*wr)->entries);

  free(*wr);
  *wr = NULL;
}

int ba_writer_add(ba_writer_t *wr, const char *entry, const void *ptr,
                  uint64_t size) {
  if (wr == NULL || ptr == NULL || size == 0) {
    errno = EINVAL;
    return -1;
  }

  struct ba_entry_column col;
  col.key = ba_hash(entry);
  col.ptr = malloc(size);
  if (col.ptr == NULL)
    return -1;
  memcpy(col.ptr, ptr, size);
  col.size = size;

  if (wr->header.entry_size >= wr->entry_cap) {
    uint32_t new_cap = wr->entry_cap << 1;
    struct ba_entry_column *new_entries =
        realloc(wr->entries, new_cap * sizeof(*wr->entries));
    if (new_entries == NULL) {
      free(col.ptr);
      return -1;
    }

    wr->entries = new_entries;
    wr->entry_cap = new_cap;
  }

  wr->entries[wr->header.entry_size++] = col;

  return 0;
}

int ba_writer_add_file(ba_writer_t *wr, const char *filename) {
  if (wr == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  struct stat st;
  if (stat(filename, &st) < 0)
    return -1;

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL)
    return -1;

  uint64_t size = st.st_size;
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fclose(fp);
    return -1;
  }

  if (fread(ptr, 1, size, fp) < size) {
    fclose(fp);
    free(ptr);
    return -1;
  }

  fclose(fp);

  if (ba_writer_add(wr, filename, ptr, size) < 0) {
    free(ptr);
    return -1;
  }

  free(ptr);

  return 0;
}

int ba_writer_write(ba_writer_t *wr, const char *filename) {
  if (wr == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  FILE *fp = fopen(filename, "wb");
  if (fp == NULL)
    return -1;

  if (fseeko(fp,
             sizeof(wr->header) +
                 wr->header.entry_size * sizeof(struct ba_entry_header),
             SEEK_SET) < 0) {
    fclose(fp);
    return -1;
  }

  struct ba_entry_header *entry_headers =
      calloc(wr->header.entry_size, sizeof(*entry_headers));
  if (entry_headers == NULL) {
    fclose(fp);
    return -1;
  }

  z_stream strm = {0};

  for (uint32_t i = 0; i < wr->header.entry_size; i++) {
    struct ba_entry_column column = wr->entries[i];
    struct ba_entry_header *header = &entry_headers[i];

    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
      free(entry_headers);
      fclose(fp);
      return -1;
    }

    uint64_t size_in = column.size;
    void *buffer_in = column.ptr;
    uint64_t size_out = deflateBound(&strm, size_in);
    void *buffer_out = malloc(size_out);
    if (buffer_out == NULL) {
      deflateEnd(&strm);
      free(entry_headers);
      fclose(fp);
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
        free(entry_headers);
        fclose(fp);
        return -1;
      }
    } while (1);

    header->key = column.key;
    header->offset = ftello(fp);
    header->o_size = strm.total_in;
    header->c_size = strm.total_out;

    if (fwrite(buffer_out, 1, header->c_size, fp) < header->c_size) {
      deflateEnd(&strm);
      free(buffer_out);
      free(entry_headers);
      fclose(fp);
      return -1;
    }

    deflateEnd(&strm);
    free(buffer_out);
  }

  if (fseek(fp, 0, SEEK_SET) < 0) {
    free(entry_headers);
    fclose(fp);
    return -1;
  }

  if (fwrite(&wr->header, sizeof(wr->header), 1, fp) < 1) {
    free(entry_headers);
    fclose(fp);
    return -1;
  }

  if (fwrite(entry_headers, sizeof(*entry_headers), wr->header.entry_size, fp) <
      wr->header.entry_size) {
    free(entry_headers);
    fclose(fp);
    return -1;
  }

  free(entry_headers);
  fclose(fp);

  return 0;
}
