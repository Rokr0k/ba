#include "config.h"
#include <ba/ba.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static void print_help(const char *arg0) {
  fprintf(stderr, "Usage: %s <OPERATION> <ARCHIVE_FILE> <ENTRIES>...\n", arg0);
  fprintf(stderr, "\n");
  fprintf(stderr, "OPERATION:\n");
  fprintf(stderr, "  h  Print helpful message.\n");
  fprintf(stderr, "  v  Print version information.\n");
  fprintf(stderr, "  c  Create archive file.\n");
  fprintf(stderr, "  l  List entries from archive file.\n");
  fprintf(stderr, "  x  Extract entries from archive file.\n");
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

#ifdef _WIN32
static int add_files(ba_writer_t *wr, const char *name) {
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;

  char qp[MAX_PATH];
  snprintf(qp, sizeof(qp), "%s\\*", name);

  hFind = FindFirstFile(qp, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    perror(name);
    return -1;
  }

  do {
    if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
      continue;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", name, ffd.cFileName);

    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      add_files(wr, path);
    } else {
      if (ba_writer_add_file(wr, path) < 0) {
        FindClose(hFind);
        perror(path);
        return -1;
      }
    }
  } while (FindNextFile(hFind, &ffd) != 0);

  FindClose(hFind);

  return 0;
}
#else
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
      if (ba_writer_add_file(wr, path) < 0) {
        perror(path);
        continue;
      }

      perror(path);
    }
  }

  closedir(dir);

  return 0;
}
#endif

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

    if (ba_writer_write_file(wr, argv[2]) < 0) {
      perror("ba_writer_write");
      exit(1);
    }

    ba_writer_free(&wr);

    exit(0);
  }

  case 'l': {
    if (argc < 3) {
      print_help(argv[0]);
      exit(1);
    }

    ba_reader_t *rd;
    if (ba_reader_alloc(&rd) < 0) {
      perror("ba_reader_alloc");
      exit(1);
    }

    if (ba_reader_open_file(rd, argv[2]) < 0) {
      perror(argv[2]);
      ba_reader_free(&rd);
      exit(1);
    }

    fprintf(stdout, "Entry Name: Size\n");

    uint32_t size = ba_reader_size(rd);
    for (ba_id_t id = 0; id < size; id++) {
      const char *name;
      uint64_t len;
      ba_reader_entry_name(rd, id, &name, &len);
      uint64_t esize = ba_reader_entry_size(rd, id);

      fprintf(stdout, "\"%.*s\": %llu\n", (int)len, name, esize);
    }

    ba_reader_free(&rd);
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

    if (ba_reader_open_file(rd, argv[2]) < 0) {
      perror(argv[2]);
      ba_reader_free(&rd);
      exit(1);
    }

    for (int i = 3; i < argc; i++) {
      ba_id_t id = ba_reader_find_entry(rd, argv[i], 0);
      if (id == BA_ENTRY_INVALID) {
        perror(argv[i]);
        continue;
      }

      uint64_t size = ba_reader_entry_size(rd, id);
      if (size == 0) {
        perror(argv[i]);
        continue;
      }
      void *ptr = malloc(size);
      if (ptr == NULL) {
        perror(argv[i]);
        continue;
      }

      if (ba_reader_read(rd, id, ptr) < 0) {
        free(ptr);
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
        free(ptr);
        perror(name);
        continue;
      }

      if (fwrite(ptr, 1, size, fp) < size) {
        fclose(fp);
        free(ptr);
        perror(name);
        continue;
      }

      fclose(fp);
      free(ptr);

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
