#include "tree.h"
#include <stdlib.h>

int ba_tree_find(ba_source_t *src, uint64_t root, uint16_t node_size,
                 uint16_t depth, uint64_t query, uint64_t *result) {
  if (src == NULL || root == 0 || result == NULL)
    return -1;

  if (ba_source_seek(src, root) < 0)
    return -1;

  size_t data_size = node_size * 2 + 1;

  uint64_t *data = calloc(data_size, sizeof(uint64_t));
  if (data == NULL)
    return -1;

  if (ba_source_read(src, data, data_size * sizeof(uint64_t)) <
      data_size * sizeof(uint64_t)) {
    free(data);
    return -1;
  }

  if (depth > 0) {
    for (uint16_t i = 0; i < node_size; i++) {
      if (data[i * 2 + 1] == 0 || query < data[i * 2 + 1]) {
        uint64_t ptr = data[i * 2];
        free(data);
        return ba_tree_find(src, ptr, node_size, depth - 1, query, result);
      }
    }

    uint64_t ptr = data[node_size * 2];
    free(data);
    return ba_tree_find(src, ptr, node_size, depth - 1, query, result);
  } else {
    for (uint64_t i = 0; i < node_size; i++) {
      if (query == data[i * 2 + 1]) {
        *result = data[i * 2];
        free(data);
        return 0;
      }
    }

    uint64_t ptr = data[node_size * 2];
    free(data);
    return ba_tree_find(src, ptr, node_size, depth, query, result);
  }
}
