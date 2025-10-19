#ifndef BA_READER_HPP
#define BA_READER_HPP

#include "reader.h"
#include <string>

namespace ba {
class Reader {
public:
  Reader() : rd(nullptr) { ba_reader_alloc(&rd); }

  Reader(Reader &&rhs) noexcept : rd(nullptr) {
    rd = rhs.rd;
    rhs.rd = nullptr;
  }

  Reader &operator=(Reader &&rhs) noexcept {
    if (this == &rhs)
      return *this;
    ba_reader_free(&rd);
    rd = rhs.rd;
    rhs.rd = nullptr;
    return *this;
  }

  ~Reader() { ba_reader_free(&rd); }

  bool Open(const std::string &filename) {
    return ba_reader_open(rd, filename.c_str()) == 0;
  }

  uint64_t GetSize(const std::string &entry) const {
    return ba_reader_size(rd, entry.c_str());
  }

  bool Read(const std::string &entry, void *ptr) {
    return ba_reader_read(rd, entry.c_str(), ptr) == 0;
  }

private:
  ba_reader_t *rd;
};
} // namespace ba

#endif
