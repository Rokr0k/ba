#include <ba/archive.h>

int main(int argc, char **argv) {
  if (argc < 2)
    return 1;

  ba_archive_t *ar;
  if (ba_archive_alloc(&ar) < 0)
    return 1;

  ba_source_t src;
  if (ba_source_init_file(&src, argv[1]) < 0)
    return 1;

  if (ba_archive_open(ar, &src) < 0) {
    ba_archive_free(&ar);
    return 1;
  }

  ba_archive_free(&ar);

  return 0;
}
