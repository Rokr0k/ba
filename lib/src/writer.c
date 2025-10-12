#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/writer.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct ba_entry_column {
  uint64_t key;
  ba_source_t src;
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
    ba_source_free(&(*wr)->entries[i].src);

  free((*wr)->entries);

  free(*wr);
  *wr = NULL;
}

int ba_writer_add(ba_writer_t *wr, const char *entry, ba_source_t *src) {
  if (wr == NULL || entry == NULL || src == NULL) {
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

  wr->entries[wr->header.entry_size].key = ba_hash(entry);
  wr->entries[wr->header.entry_size].src = *src;

  ba_source_init(src);

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

  uint32_t errors = 0;
  for (uint32_t i = 0; i < wr->header.entry_size; i++) {
    if (ba_source_seek(&wr->entries[i + errors].src, 0, SEEK_SET) < 0) {
      i--;
      wr->header.entry_size--;
      errors++;
      continue;
    }

    char buffer[1 << 16];
    size_t read;
    uint64_t cr = 0;
    while ((read = ba_source_read(&wr->entries[i + errors].src, buffer,
                                  sizeof(buffer))) > 0) {
      if (ba_source_write(src, buffer, read) < 0) {
        free(entries);
        return -1;
      }

      cr += read;
    }

    entries[i].key = wr->entries[i + errors].key;
    entries[i].offset = content_offset;
    entries[i].size = cr;

    content_offset += cr;
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
