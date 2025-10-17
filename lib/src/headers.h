#ifndef BA_HEADERS_H
#define BA_HEADERS_H

#include <stdint.h>

struct ba_archive_header {
  uint32_t signature;
  uint32_t entry_size;
};

struct ba_entry_header {
  uint64_t key;
  uint64_t offset;
  uint64_t e_size;
  uint64_t o_size;
};

#endif
