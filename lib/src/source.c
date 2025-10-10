#include <ba/source.h>
#include <stdlib.h>
#include <string.h>

static void fp_free(void *arg) {
  FILE *fp = arg;
  fclose(fp);
}

static int fp_seek(void *arg, uint64_t offset) {
  FILE *fp = arg;

  if (offset >= 0)
    return fseek(fp, offset, SEEK_SET);
  else
    return fseek(fp, -offset + 1, SEEK_END);
}

static uint64_t fp_read(void *arg, void *ptr, uint64_t size) {
  FILE *fp = arg;

  uint64_t cr = 0;
  uint64_t read;
  while (size > 0 && (read = fread(ptr, 1, size, fp)) > 0) {
    ptr += read;
    size -= read;
    cr += read;
  }

  return cr;
}

struct mem_context {
  void *ptr;
  uint64_t size;
  uint64_t cursor;
  void (*dealloc)(void *);
};

static void mem_free(void *arg) {
  struct mem_context *ctx = arg;

  ctx->dealloc(ctx->ptr);

  free(ctx);
}

static int mem_seek(void *arg, uint64_t offset) {
  struct mem_context *ctx = arg;

  if (offset >= 0) {
    if (offset > ctx->size)
      return -1;
    ctx->cursor = offset;
  } else {
    if (ctx->size + offset + 1 < 0)
      return -1;
    ctx->cursor = ctx->size + offset + 1;
  }
  return 0;
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

struct constmem_context {
  const void *ptr;
  uint64_t size;
  uint64_t cursor;
};

static void constmem_free(void *arg) {
  struct constmem_context *ctx = arg;

  free(ctx);
}

static int constmem_seek(void *arg, uint64_t offset) {
  struct constmem_context *ctx = arg;

  if (offset >= 0) {
    if (offset > ctx->size)
      return -1;
    ctx->cursor = offset;
  } else {
    if (ctx->size + offset + 1 < 0)
      return -1;
    ctx->cursor = ctx->size + offset + 1;
  }
  return 0;
}

static uint64_t constmem_read(void *arg, void *ptr, uint64_t size) {
  struct constmem_context *ctx = arg;

  if (ctx->cursor + size > ctx->size)
    size = ctx->size - ctx->cursor;

  if (size == 0)
    return 0;

  memcpy(ptr, (const char *)ctx->ptr + ctx->cursor, size);
  ctx->cursor += size;

  return size;
}

int ba_source_init_file(ba_source_t *src, const char *filename) {
  if (src == NULL || filename == NULL)
    return -1;

  FILE *fp = fopen(filename, "rb");
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
  src->read = fp_read;
  src->arg = fp;
}

int ba_source_init_mem(ba_source_t *src, void *ptr, uint64_t size,
                       void (*dealloc)(void *)) {
  if (src == NULL || ptr == NULL || size == 0)
    return -1;

  if (dealloc == NULL)
    dealloc = free;

  struct mem_context *ctx = malloc(sizeof(*ctx));
  if (ctx == NULL)
    return -1;

  ctx->ptr = ptr;
  ctx->size = size;
  ctx->cursor = 0;
  ctx->dealloc = dealloc;

  src->free = mem_free;
  src->seek = mem_seek;
  src->read = mem_read;
  src->arg = ctx;

  return 0;
}

int ba_source_init_constmem(ba_source_t *src, const void *ptr, uint64_t size) {
  if (src == NULL || ptr == NULL || size == 0)
    return -1;

  struct constmem_context *ctx = malloc(sizeof(*ctx));
  if (ctx == NULL)
    return -1;

  ctx->ptr = ptr;
  ctx->size = size;
  ctx->cursor = 0;

  src->free = mem_free;
  src->seek = mem_seek;
  src->read = mem_read;
  src->arg = ctx;

  return 0;
}

void ba_source_free(ba_source_t *src) {
  if (src == NULL || src->free == NULL)
    return;

  src->free(src->arg);
}

int ba_source_seek(ba_source_t *src, uint64_t offset) {
  if (src == NULL || src->seek == NULL)
    return -1;

  return src->seek(src->arg, offset);
}

size_t ba_source_read(ba_source_t *src, void *ptr, uint64_t size) {
  if (src == NULL || src->read == NULL)
    return 0;

  return src->read(src->arg, ptr, size);
}
