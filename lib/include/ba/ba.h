#ifndef BA_BA_H
#define BA_BA_H

#include <ba/exports.h>
#include <ba/reader.h>
#include <ba/writer.h>

#define BA_MAKE_VERSION(major, minor, patch)                                   \
  ((((major) & 0xff) << 24) | (((minor) & 0xff) << 16) |                       \
   (((patch) & 0xff) << 8))

#define BA_VERSION_MAJOR(version) ((version >> 24) & 0xff)
#define BA_VERSION_MINOR(version) ((version >> 16) & 0xff)
#define BA_VERSION_PATCH(version) ((version >> 8) & 0xff)

BA_API uint32_t ba_version(void);

#endif
