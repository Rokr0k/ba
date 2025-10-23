#include <ba/buffer.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct ba_buffer {
  void (*free)(void *arg);
  int (*seek)(void *arg, int64_t pos, int whence);
  int64_t (*tell)(void *arg);
  uint64_t (*read)(void *arg, void *ptr, uint64_t size);
  int (*write)(void *arg, const void *ptr, uint64_t size);
  uint64_t (*size)(void *arg);

  void *arg;
};

#define BA_BUF_ASSI(buf, pfx, wrd) (buf)->wrd = pfx##_##wrd;

#define BA_BUF_INIT(buf, pfx)                                                  \
  do {                                                                         \
    BA_BUF_ASSI(buf, pfx, free)                                                \
    BA_BUF_ASSI(buf, pfx, seek)                                                \
    BA_BUF_ASSI(buf, pfx, tell)                                                \
    BA_BUF_ASSI(buf, pfx, read)                                                \
    BA_BUF_ASSI(buf, pfx, write)                                               \
    BA_BUF_ASSI(buf, pfx, size)                                                \
  } while (0)

struct ba_buffer_ctx_mem {
  void *ptr;
  uint64_t size;
  uint64_t curr;
};

static void mem_free(void *arg) {
  struct ba_buffer_ctx_mem *ctx = arg;

  free(ctx->ptr);
  free(ctx);
}

static int mem_seek(void *arg, int64_t pos, int whence) {
  struct ba_buffer_ctx_mem *ctx = arg;

  switch (whence) {
  case SEEK_SET:
    if (pos < 0) {
      errno = EINVAL;
      return -1;
    }

    if (pos > ctx->size) {
      uint64_t new_size = pos;
      void *new_ptr = realloc(ctx->ptr, new_size);
      if (new_ptr == NULL) {
        return -1;
      }
      memset(&((char *)ctx->ptr)[ctx->size], 0, new_size - ctx->size);
      ctx->ptr = new_ptr;
      ctx->size = new_size;
    }

    ctx->curr = pos;

    return 0;

  case SEEK_CUR:
    if (pos + (int64_t)ctx->curr < 0) {
      errno = EINVAL;
      return -1;
    }

    if (pos + ctx->curr > ctx->size) {
      uint64_t new_size = pos + ctx->curr;
      void *new_ptr = realloc(ctx->ptr, new_size);
      if (new_ptr == NULL) {
        return -1;
      }
      memset(&((char *)ctx->ptr)[ctx->size], 0, new_size - ctx->size);
      ctx->ptr = new_ptr;
      ctx->size = new_size;
    }

    ctx->curr = pos + ctx->curr;

    return 0;

  case SEEK_END:
    if (pos + (int64_t)ctx->size < 0) {
      errno = EINVAL;
      return -1;
    }

    if (pos > 0) {
      uint64_t new_size = pos + ctx->size;
      void *new_ptr = realloc(ctx->ptr, new_size);
      if (new_ptr == NULL) {
        return -1;
      }
      memset(&((char *)ctx->ptr)[ctx->size], 0, new_size - ctx->size);
      ctx->ptr = new_ptr;
      ctx->size = new_size;
    }

    ctx->curr = pos + ctx->size;

    return 0;

  default:
    errno = EINVAL;
    return -1;
  }
}

static int64_t mem_tell(void *arg) {
  struct ba_buffer_ctx_mem *ctx = arg;

  return ctx->curr;
}

static uint64_t mem_read(void *arg, void *ptr, uint64_t size) {
  struct ba_buffer_ctx_mem *ctx = arg;

  if (ctx->curr + size > ctx->size)
    size = ctx->size - ctx->curr;

  memcpy(ptr, &((char *)ctx->ptr)[ctx->curr], size);
  ctx->curr += size;

  return size;
}

static int mem_write(void *arg, const void *ptr, uint64_t size) {
  struct ba_buffer_ctx_mem *ctx = arg;

  if (ctx->curr + size > ctx->size) {
    uint64_t new_size = ctx->curr + size;
    void *new_ptr = realloc(ctx->ptr, new_size);
    if (new_ptr == NULL)
      return -1;
    ctx->ptr = new_ptr;
    ctx->size = new_size;
  }

  memcpy(&((char *)ctx->ptr)[ctx->curr], ptr, size);
  ctx->curr += size;

  return 0;
}

static uint64_t mem_size(void *arg) {
  struct ba_buffer_ctx_mem *ctx = arg;

  return ctx->size;
}

static void fp_free(void *arg) {
  FILE *fp = arg;

  fclose(fp);
}

static int fp_seek(void *arg, int64_t pos, int whence) {
  FILE *fp = arg;

#ifdef _WIN32
  return -(_fseeki64(fp, pos, whence) != 0);
#else
  return -(fseeko(fp, pos, whence) != 0);
#endif
}

static int64_t fp_tell(void *arg) {
  FILE *fp = arg;

#ifdef _WIN32
  return _ftelli64(fp);
#else
  return ftello(fp);
#endif
}

static uint64_t fp_read(void *arg, void *ptr, uint64_t size) {
  FILE *fp = arg;

  return fread(ptr, 1, size, fp);
}

static int fp_write(void *arg, const void *ptr, uint64_t size) {
  FILE *fp = arg;

  return -(fwrite(ptr, 1, size, fp) < size);
}

static uint64_t fp_size(void *arg) {
  FILE *fp = arg;

#ifdef _WIN32
  int fd = _fileno(fp);
  if (fd == -1)
    return 0;

  struct _stat64 stat;
  if (_fstat64(fd, &stat) < 0)
    return 0;

  return stat.st_size;
#else
  int fd = fileno(fp);
  if (fd == -1)
    return 0;

  struct stat stat;
  if (fstat(fd, &stat) < 0)
    return 0;

  return stat.st_size;
#endif
}

int ba_buffer_init(ba_buffer_t **buf) {
  if (buf == NULL) {
    errno = EINVAL;
    return -1;
  }

  *buf = calloc(1, sizeof(**buf));
  if (*buf == NULL)
    return -1;

  struct ba_buffer_ctx_mem *ctx = malloc(sizeof(*ctx));
  if (ctx == NULL) {
    free(*buf);
    return -1;
  }

  ctx->ptr = NULL;
  ctx->size = 0;
  ctx->curr = 0;

  BA_BUF_INIT(*buf, mem);
  (*buf)->arg = ctx;

  return 0;
}

int ba_buffer_init_mem(ba_buffer_t **buf, const void *ptr, uint64_t size) {
  if (buf == NULL || ptr == NULL || size == 0) {
    errno = EINVAL;
    return -1;
  }

  *buf = calloc(1, sizeof(**buf));
  if (*buf == NULL)
    return -1;

  struct ba_buffer_ctx_mem *ctx = malloc(sizeof(*ctx));
  if (ctx == NULL) {
    free(*buf);
    return -1;
  }

  ctx->ptr = malloc(ctx->size = size);
  if (ctx->ptr == NULL) {
    free(ctx);
    free(*buf);
    return -1;
  }
  memcpy(ctx->ptr, ptr, ctx->size);
  ctx->curr = 0;

  BA_BUF_INIT(*buf, mem);
  (*buf)->arg = ctx;

  return 0;
}

int ba_buffer_init_file(ba_buffer_t **buf, const char *filename,
                        const char *mode) {
  if (buf == NULL || filename == NULL) {
    errno = EINVAL;
    return -1;
  }

  *buf = calloc(1, sizeof(**buf));
  if (*buf == NULL)
    return -1;

  FILE *fp = NULL;

#ifdef _WIN32
  if (fopen_s(&fp, filename, mode) != 0) {
    free(*buf);
    return -1;
  }
#else
  fp = fopen(filename, mode);
  if (fp == NULL) {
    free(*buf);
    return -1;
  }
#endif

  BA_BUF_INIT(*buf, fp);
  (*buf)->arg = fp;

  return 0;
}

void ba_buffer_free(ba_buffer_t **buf) {
  if (buf == NULL || *buf == NULL)
    return;

  if ((*buf)->free == NULL)
    return;

  (*buf)->free((*buf)->arg);

  free(*buf);
  *buf = NULL;
}

int ba_buffer_seek(ba_buffer_t *buf, int64_t pos, int whence) {
  if (buf == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (buf->seek == NULL) {
    errno = EOPNOTSUPP;
    return -1;
  }

  return buf->seek(buf->arg, pos, whence);
}

int64_t ba_buffer_tell(ba_buffer_t *buf) {
  if (buf == NULL) {
    errno = EINVAL;
    return -1LL;
  }

  if (buf->tell == NULL) {
    errno = EOPNOTSUPP;
    return -1LL;
  }

  return buf->tell(buf->arg);
}

uint64_t ba_buffer_read(ba_buffer_t *buf, void *ptr, uint64_t size) {
  if (buf == NULL || ptr == NULL) {
    errno = EINVAL;
    return ~0ULL;
  }

  if (size == 0)
    return 0;

  if (buf->read == NULL) {
    errno = EOPNOTSUPP;
    return ~0ULL;
  }

  return buf->read(buf->arg, ptr, size);
}

int ba_buffer_write(ba_buffer_t *buf, const void *ptr, uint64_t size) {
  if (buf == NULL || ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (size == 0)
    return 0;

  if (buf->write == NULL) {
    errno = EOPNOTSUPP;
    return -1;
  }

  return buf->write(buf->arg, ptr, size);
}

uint64_t ba_buffer_size(ba_buffer_t *buf) {
  if (buf == NULL) {
    errno = EINVAL;
    return 0;
  }

  if (buf->size == NULL) {
    errno = EOPNOTSUPP;
    return 0;
  }

  return buf->size(buf->arg);
}
