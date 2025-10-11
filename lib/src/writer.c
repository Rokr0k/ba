#include "hash.h"
#include "headers.h"
#include "signature.h"
#include <ba/writer.h>
#include <stdlib.h>
#include <string.h>

struct ba_writer {
  struct ba_archive_header header;
  uint32_t entry_cap;
  char **entries;
};

int ba_writer_alloc(ba_writer_t **wr) {
  if (wr == NULL)
    return -1;

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
  if (wr == NULL || *wr == NULL)
    return;

  for (uint32_t i = 0; i < (*wr)->header.entry_size; i++)
    free((*wr)->entries[i]);

  free((*wr)->entries);

  free(*wr);
  *wr = NULL;
}

int ba_writer_add(ba_writer_t *wr, const char *entry) {
  if (wr == NULL || entry == NULL)
    return -1;

  if (wr->header.entry_size >= wr->entry_cap) {
    uint32_t new_cap = wr->entry_cap << 1;
    char **new_entries = realloc(wr->entries, new_cap);
    if (new_entries == NULL)
      return -1;

    wr->entries = new_entries;
    wr->entry_cap = new_cap;
  }

  size_t len = strlen(entry);

  wr->entries[wr->header.entry_size] = malloc(len);
  if (wr->entries[wr->header.entry_size] == NULL)
    return -1;

  strncpy(wr->entries[wr->header.entry_size], entry, len);

  wr->header.entry_size++;

  return 0;
}

int ba_writer_write(ba_writer_t *wr, ba_source_t *src) {
  if (wr == NULL || src == NULL)
    return -1;

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
    FILE *fp = fopen(wr->entries[i + errors], "rb");
    if (fp == NULL) {
      i--;
      wr->header.entry_size--;
      errors++;
      continue;
    }

    char buffer[1 << 16];
    size_t read;
    uint64_t cr = 0;
    while ((read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
      if (ba_source_write(src, buffer, read) < 0) {
        free(entries);
        return -1;
      }

      cr += read;
    }

    fclose(fp);

    entries[i].key = ba_hash(wr->entries[i + errors]);
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
