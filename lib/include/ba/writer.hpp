#ifndef BA_WRITER_HPP
#define BA_WRITER_HPP

#include "buffer.hpp"
#include "writer.h"

namespace ba {
class Writer {
public:
  Writer() : wr(nullptr) {}

  Writer(Writer &&rhs) noexcept : wr(rhs.wr) { rhs.wr = nullptr; }

  Writer &operator=(Writer &&rhs) noexcept {
    if (this != &rhs) {
      ba_writer_free(&wr);
      wr = rhs.wr;
      rhs.wr = nullptr;
    }
    return *this;
  }

  ~Writer() { ba_writer_free(&wr); }

  operator bool() const { return wr != nullptr; }

  bool operator!() const { return wr == nullptr; }

  bool Add(const std::string &entry, Buffer &&buf) {
    int ret = ba_writer_add(wr, entry.c_str(), entry.length(), buf.buf);
    buf.buf = nullptr;
    return ret == 0;
  }

  bool Add(const std::string &filename) {
    return ba_writer_add_file(wr, filename.c_str()) == 0;
  }

  bool Write(Buffer &buf) { return ba_writer_write(wr, buf.buf) == 0; }

  bool Write(const std::string &filename) {
    return ba_writer_write_file(wr, filename.c_str()) == 0;
  }

private:
  ba_writer_t *wr;
};
} // namespace ba

#endif
