#ifndef BA_TREE_H
#define BA_TREE_H

#include <ba/source.h>
#include <stdint.h>

int ba_tree_find(ba_source_t *src, uint64_t root, uint16_t node_size,
                 uint16_t depth, uint64_t query, uint64_t *result);

#endif
