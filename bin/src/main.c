#include "config.h"
#include <ba/ba.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void print_help(const char *arg0) {
  fprintf(stderr, "Usage: %s <OPERATION> <ARCHIVE_FILE> <ENTRIES>...\n", arg0);
  fprintf(stderr, "\n");
  fprintf(stderr, "OPERATION:\n");
  fprintf(stderr, "  h  Print helpful message.\n");
  fprintf(stderr, "  v  Print version information.\n");
  fprintf(stderr, "  c  Create archive file.\n");
  fprintf(stderr, "  x  Extract from archive file.\n");
  fprintf(stderr, "\n");
}

static void print_version(const char *arg0) {
  uint32_t bin_version = BA_MAKE_VERSION(
      BA_BIN_VERSION_MAJOR, BA_BIN_VERSION_MINOR, BA_BIN_VERSION_PATCH);
  uint32_t lib_version = ba_version();

  fprintf(stderr, "%s: %d.%d.%d\n", arg0, BA_VERSION_MAJOR(bin_version),
          BA_VERSION_MINOR(bin_version), BA_VERSION_PATCH(bin_version));
  fprintf(stderr, "LIB: %d.%d.%d\n", BA_VERSION_MAJOR(lib_version),
          BA_VERSION_MINOR(lib_version), BA_VERSION_PATCH(lib_version));
}

static int add_files(ba_writer_t *wr, const char *name) {
  DIR *dir = opendir(name);
  if (dir == NULL) {
    perror(name);
    return -1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);

    struct stat st;
    if (stat(path, &st) < 0) {
      perror(path);
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      add_files(wr, path);
    } else {
      ba_source_t src;
      if (ba_source_init_file(&src, path) < 0) {
        perror(path);
        continue;
      }

      if (ba_writer_add(wr, path) < 0) {
        perror(path);
        continue;
      }

      perror(path);
    }
  }

  closedir(dir);

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_help(argv[0]);
    exit(0);
  }

  switch (argv[1][0]) {
  case 'h':
    print_help(argv[0]);
    exit(0);

  case 'v':
    print_version(argv[0]);
    exit(0);

  case 'c': {
    if (argc < 3) {
      print_help(argv[0]);
      exit(1);
    }

    ba_writer_t *wr = NULL;
    if (ba_writer_alloc(&wr) < 0) {
      perror("ba_writer_alloc");
      exit(1);
    }

    for (int i = 3; i < argc; i++) {
      if (add_files(wr, argv[i]) < 0) {
        continue;
      }
    }

    FILE *fp = fopen(argv[2], "wb");
    if (fp == NULL) {
      perror(argv[2]);
      exit(1);
    }

    ba_source_t src;
    ba_source_init_fp(&src, fp);

    if (ba_writer_write(wr, &src) < 0) {
      perror("ba_writer_write");
      exit(1);
    }

    ba_writer_free(&wr);

    exit(0);
  }

  case 'x': {
    if (argc < 3) {
      print_help(argv[0]);
      exit(1);
    }

    ba_reader_t *rd;
    if (ba_reader_alloc(&rd) < 0) {
      perror("ba_reader_alloc");
      exit(1);
    }

    ba_source_t src;
    if (ba_source_init_file(&src, argv[2]) < 0) {
      perror(argv[2]);
      exit(1);
    }

    if (ba_reader_open(rd, &src) < 0) {
      perror(argv[2]);
      exit(1);
    }

    for (int i = 3; i < argc; i++) {
      void *ptr;
      uint64_t size;
      if (ba_reader_read(rd, argv[i], &ptr, &size) < 0) {
        perror(argv[i]);
        continue;
      }

      char *name = strrchr(argv[i], '/');
      if (name == NULL)
        name = argv[i];
      else
        name++;

      FILE *fp = fopen(name, "wb");
      if (fp == NULL) {
        perror(name);
        continue;
      }

      ba_source_init_fp(&src, fp);

      if (ba_source_write(&src, ptr, size) < 0) {
        perror(argv[i]);
        continue;
      }

      ba_source_free(&src);

      perror(argv[i]);
    }

    ba_reader_free(&rd);

    exit(0);
  }

  default:
    fprintf(stderr, "Unknown operation: '%c'.\n", argv[1][0]);
    print_help(argv[0]);
    exit(1);
  }
}
