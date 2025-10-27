#ifndef BA_BUFFER_HPP
#define BA_BUFFER_HPP

#include "buffer.h"
#include <string>

namespace ba {
class Buffer {
public:
  Buffer() : buf(nullptr) {}

  Buffer(Buffer &&rhs) noexcept : buf(rhs.buf) { rhs.buf = nullptr; }

  Buffer &operator=(Buffer &&rhs) noexcept {
    if (this != &rhs) {
      ba_buffer_free(&buf);
      buf = rhs.buf;
      rhs.buf = nullptr;
    }
    return *this;
  }

  ~Buffer() { ba_buffer_free(&buf); }

  operator bool() const { return buf != nullptr; }

  bool operator!() const { return buf == nullptr; }

  bool Init() { return ba_buffer_init(&buf) == 0; }

  bool Init(const void *ptr, uint64_t size) {
    return ba_buffer_init_mem(&buf, ptr, size) == 0;
  }

  bool Init(const std::string &filename, const std::string &mode) {
    return ba_buffer_init_file(&buf, filename.c_str(), mode.c_str());
  }

  bool Seek(int64_t pos, int whence) {
    return ba_buffer_seek(buf, pos, whence) == 0;
  }

  int64_t Tell() { return ba_buffer_tell(buf); }

  uint64_t Read(void *ptr, uint64_t size) {
    return ba_buffer_read(buf, ptr, size);
  }

  int Write(const void *ptr, uint64_t size) {
    return ba_buffer_write(buf, ptr, size) == 0;
  }

  uint64_t Size() { return ba_buffer_size(buf); }

private:
  ba_buffer_t *buf;

  friend class Reader;
  friend class Writer;
};
} // namespace ba

#endif
