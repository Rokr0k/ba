#include "hash.h"
#include <string.h>

uint64_t ba_hash(const char *str) {
  uint64_t h = 0;

  int zero = 0;

  while (!zero) {
    uint64_t n = 0;
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
      if (!zero && str[i] == '\0')
        zero = 1;

      if (!zero)
        n |= str[i];

      n >>= sizeof(uint64_t);
    }

    h ^= n;

    str += 8;
  }

  h |= (1ULL << 63);

  return h;
}
