#include <ba/source.h>
#include <stdlib.h>
#include <string.h>

static void fp_free(void *arg) {
  FILE *fp = arg;
  fclose(fp);
}

static int fp_seek(void *arg, int64_t offset, int whence) {
  FILE *fp = arg;

  if (fseeko(fp, offset, whence) != 0)
    return -1;

  return 0;
}

static int64_t fp_tell(void *arg) {
  FILE *fp = arg;

  return ftello(fp);
}

static uint64_t fp_read(void *arg, void *ptr, uint64_t size) {
  FILE *fp = arg;

  uint64_t cr = 0;
  uint64_t read;
  while (size > 0 && (read = fread(ptr, 1, size, fp)) > 0) {
    ptr = (char *)ptr + read;
    size -= read;
    cr += read;
  }

  return cr;
}

static int fp_write(void *arg, const void *ptr, uint64_t size) {
  FILE *fp = arg;

  if (fwrite(ptr, 1, size, fp) < size)
    return -1;
  else
    return 0;
}

struct mem_context {
  void *ptr;
  uint64_t size;
  uint64_t cursor;
};

static void mem_free(void *arg) {
  struct mem_context *ctx = arg;

  free(ctx->ptr);
  free(ctx);
}

static int mem_seek(void *arg, int64_t offset, int whence) {
  struct mem_context *ctx = arg;

  switch (whence) {
  case SEEK_SET:
    if (offset < 0)
      return -1;

    if (offset > ctx->size) {
      void *new_ptr = realloc(ctx->ptr, offset);
      if (new_ptr == NULL)
        return -1;

      memset((char *)new_ptr + ctx->size, 0, offset - ctx->size);

      ctx->ptr = new_ptr;
      ctx->size = offset;
    }

    ctx->cursor = offset;

    return 0;

  case SEEK_CUR:
    if (ctx->cursor < -offset)
      return -1;

    if (offset + ctx->cursor > ctx->size) {
      void *new_ptr = realloc(ctx->ptr, offset + ctx->cursor);
      if (new_ptr == NULL)
        return -1;

      memset((char *)new_ptr + ctx->size, 0, offset + ctx->cursor - ctx->size);

      ctx->ptr = new_ptr;
      ctx->size = offset + ctx->cursor;
    }

    ctx->cursor += offset;

    return 0;

  case SEEK_END:
    if (ctx->size < -offset)
      return -1;

    if (offset > 0) {
      void *new_ptr = realloc(ctx->ptr, offset + ctx->size);
      if (new_ptr == NULL)
        return -1;

      memset((char *)new_ptr + ctx->size, 0, offset);

      ctx->ptr = new_ptr;
      ctx->size += offset;
    }

    ctx->cursor = offset + ctx->size;

    return 0;

  default:
    return -1;
  }
}

static int64_t mem_tell(void *arg) {
  struct mem_context *ctx = arg;

  return ctx->cursor;
}

static uint64_t mem_read(void *arg, void *ptr, uint64_t size) {
  struct mem_context *ctx = arg;

  if (ctx->cursor + size > ctx->size)
    size = ctx->size - ctx->cursor;

  if (size == 0)
    return 0;

  memcpy(ptr, (const char *)ctx->ptr + ctx->cursor, size);
  ctx->cursor += size;

  return size;
}

static int mem_write(void *arg, const void *ptr, uint64_t size) {
  struct mem_context *ctx = arg;

  if (ctx->cursor + size > ctx->size) {
    void *new_ptr = realloc(ctx->ptr, ctx->cursor + size);
    if (new_ptr == NULL)
      return -1;
    ctx->ptr = new_ptr;
    ctx->size = ctx->cursor + size;
  }

  memcpy((char *)ctx->ptr + ctx->cursor, ptr, size);
  ctx->cursor += size;

  return 0;
}

int ba_source_init_file(ba_source_t *src, const char *filename) {
  if (src == NULL || filename == NULL)
    return -1;

  FILE *fp = fopen(filename, "r+b");
  if (fp == NULL)
    return -1;

  ba_source_init_fp(src, fp);

  return 0;
}

void ba_source_init_fp(ba_source_t *src, FILE *fp) {
  if (src == NULL || fp == NULL)
    return;

  src->free = fp_free;
  src->seek = fp_seek;
  src->tell = fp_tell;
  src->read = fp_read;
  src->write = fp_write;
  src->arg = fp;
}

int ba_source_init_mem(ba_source_t *src, const void *ptr, uint64_t size) {
  if (src == NULL || ptr == NULL || size == 0)
    return -1;

  struct mem_context *ctx = malloc(sizeof(*ctx));
  if (ctx == NULL)
    return -1;

  void *buf = malloc(size);
  if (buf == NULL) {
    free(ctx);
    return -1;
  }
  memcpy(buf, ptr, size);

  ctx->ptr = buf;
  ctx->size = size;
  ctx->cursor = 0;

  src->free = mem_free;
  src->seek = mem_seek;
  src->tell = mem_tell;
  src->read = mem_read;
  src->write = mem_write;
  src->arg = ctx;

  return 0;
}

void ba_source_free(ba_source_t *src) {
  if (src == NULL || src->free == NULL)
    return;

  src->free(src->arg);

  src->free = NULL;
  src->seek = NULL;
  src->tell = NULL;
  src->read = NULL;
  src->write = NULL;
  src->arg = NULL;
}

int ba_source_seek(ba_source_t *src, int64_t offset, int whence) {
  if (src == NULL || src->seek == NULL)
    return -1;

  return src->seek(src->arg, offset, whence);
}

int64_t ba_source_tell(ba_source_t *src) {
  if (src == NULL || src->tell == NULL)
    return -1;

  return src->tell(src->arg);
}

size_t ba_source_read(ba_source_t *src, void *ptr, uint64_t size) {
  if (src == NULL || src->read == NULL)
    return 0;

  return src->read(src->arg, ptr, size);
}

int ba_source_write(ba_source_t *src, const void *ptr, uint64_t size) {
  if (src == NULL || src->write == NULL)
    return -1;

  return src->write(src->arg, ptr, size);
}
