#ifndef BA_WRITER_HPP
#define BA_WRITER_HPP

#include "writer.h"
#include <string>

namespace ba {
class Writer {
public:
  Writer() : wr(nullptr) { ba_writer_alloc(&wr); }

  Writer(Writer &&rhs) noexcept : wr(nullptr) {
    wr = rhs.wr;
    rhs.wr = nullptr;
  }

  Writer &operator=(Writer &&rhs) noexcept {
    if (this == &rhs)
      return *this;
    ba_writer_free(&wr);
    wr = rhs.wr;
    rhs.wr = nullptr;
    return *this;
  }

  ~Writer() { ba_writer_free(&wr); }

  bool Add(const std::string &entry, const void *ptr, uint64_t size) {
    return ba_writer_add(wr, entry.c_str(), ptr, size) == 0;
  }

  bool Add(const std::string &filename) {
    return ba_writer_add_file(wr, filename.c_str()) == 0;
  }

  bool Write(const std::string &filename) {
    return ba_writer_write(wr, filename.c_str()) == 0;
  }

private:
  ba_writer_t *wr;
};
} // namespace ba

#endif
