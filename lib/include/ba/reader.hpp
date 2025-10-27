#ifndef BA_READER_HPP
#define BA_READER_HPP

#include "buffer.hpp"
#include "reader.h"

namespace ba {
class Reader {
public:
  Reader() : rd(nullptr) {}

  Reader(Reader &&rhs) noexcept : rd(rhs.rd) { rhs.rd = nullptr; }

  Reader &operator=(Reader &&rhs) noexcept {
    if (this != &rhs) {
      ba_reader_free(&rd);
      rd = rhs.rd;
      rhs.rd = nullptr;
    }
    return *this;
  }

  ~Reader() { ba_reader_free(&rd); }

  operator bool() const { return rd != nullptr; }

  bool operator!() const { return rd == nullptr; }

  bool Open(Buffer &buf) { return ba_reader_open(rd, buf.buf) == 0; }

  bool Open(const std::string &filename) {
    return ba_reader_open_file(rd, filename.c_str()) == 0;
  }

  uint32_t Size() const { return ba_reader_size(rd); }

  ba_id_t FindEntry(const std::string &entry) const {
    return ba_reader_find_entry(rd, entry.c_str(), entry.length());
  }

  std::string EntryName(ba_id_t id) const {
    const char *str;
    uint64_t len;
    if (ba_reader_entry_name(rd, id, &str, &len) != 0)
      return "";
    return {str, len};
  }

  uint64_t EntrySize(ba_id_t id) const { return ba_reader_entry_size(rd, id); }

  bool Read(ba_id_t id, void *ptr) { return ba_reader_read(rd, id, ptr) == 0; }

private:
  ba_reader_t *rd;
};
} // namespace ba

#endif
