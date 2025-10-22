#ifndef BA_HEADERS_H
#define BA_HEADERS_H

#include <stdint.h>

struct ba_archive_header {
  uint32_t sign;
  uint32_t ensz;
  uint64_t tbsz;
};

struct ba_entry_header {
  uint64_t tidx;
  uint64_t tlen;
  uint64_t boff;
  uint64_t bcsz;
  uint64_t bosz;
};

#endif
