#include <ba/source.h>
#include <stdlib.h>
#include <string.h>

void ba_source_init(ba_source_t *src) {
  if (src == NULL)
    return;

  src->type = BA_SOURCE_UNK;
}

void ba_source_init_fp(ba_source_t *src, FILE *fp) {
  if (src == NULL)
    return;

  src->fp = (struct ba_source_fp){
      .type = BA_SOURCE_FP,
      .fp = fp,
  };
}

void ba_source_init_mem(ba_source_t *src, void *ptr, size_t size,
                        void (*mesiah)(void *)) {
  if (src == NULL)
    return;

  if (mesiah == NULL)
    mesiah = free;

  src->mem = (struct ba_source_mem){
      .type = BA_SOURCE_MEM,
      .ptr = ptr,
      .size = size,
      .cursor = 0,
      .mesiah = mesiah,
  };
}

void ba_source_init_constmem(ba_source_t *src, const void *ptr, size_t size) {
  if (src == NULL)
    return;

  src->constmem = (struct ba_source_constmem){
      .type = BA_SOURCE_CONSTMEM,
      .ptr = ptr,
      .size = size,
      .cursor = 0,
  };
}

void ba_source_free(ba_source_t *src) {
  if (src == NULL)
    return;

  switch (src->type) {
  case BA_SOURCE_FP:
    fclose(src->fp.fp);
    break;

  case BA_SOURCE_MEM:
    src->mem.mesiah(src->mem.ptr);
    break;
  }

  ba_source_init(src);
}

size_t ba_source_seek(ba_source_t *src, off_t offset) {
  if (src == NULL)
    return -1;

  switch (src->type) {
  case BA_SOURCE_FP: {
    int whence = SEEK_SET;

    if (offset < 0) {
      offset = -offset;
      whence = SEEK_END;
    }

    if (fseek(src->fp.fp, offset, whence))
      return -1;

    return ba_source_tell(src);
  }

  case BA_SOURCE_MEM: {
    if (offset < 0)
      offset = src->mem.size + offset;

    if (offset < 0 || offset >= src->mem.size)
      return -1;

    src->mem.cursor = offset;

    return 0;
  }

  case BA_SOURCE_CONSTMEM: {
    if (offset < 0)
      offset = src->constmem.size + offset;

    if (offset < 0 || offset >= src->constmem.size)
      return -1;

    src->constmem.cursor = offset;

    return 0;
  }

  default:
    return -1;
  }
}

size_t ba_source_tell(ba_source_t *src) {
  if (src == NULL)
    return -1;

  switch (src->type) {
  case BA_SOURCE_FP: {
    long res = ftell(src->fp.fp);
    if (res == -1L)
      return -1;
    return res;
  }

  case BA_SOURCE_MEM:
    return src->mem.cursor;

  case BA_SOURCE_CONSTMEM:
    return src->constmem.cursor;

  default:
    return -1;
  }
}

size_t ba_source_read(ba_source_t *src, void *ptr, size_t size) {
  if (src == NULL || ptr == NULL)
    return -1;

  if (size == 0)
    return 0;

  switch (src->type) {
  case BA_SOURCE_FP: {
    size_t read = fread(ptr, 1, size, src->fp.fp);

    if (read == 0 && ferror(src->fp.fp))
      return -1;

    return read;
  }

  case BA_SOURCE_MEM: {
    size_t read = size;
    if (src->mem.cursor + read > src->mem.size)
      read = src->mem.size - src->mem.cursor;

    if (read == 0)
      return 0;

    memcpy(ptr, src->mem.ptr + src->mem.cursor, read);

    src->mem.cursor += read;

    return read;
  }

  case BA_SOURCE_CONSTMEM: {
    size_t read = size;
    if (src->constmem.cursor + read > src->constmem.size)
      read = src->constmem.size - src->constmem.cursor;

    if (read == 0)
      return 0;

    memcpy(ptr, src->constmem.ptr + src->constmem.cursor, read);

    src->constmem.cursor += read;

    return read;
  }

  default:
    return -1;
  }
}
