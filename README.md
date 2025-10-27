# BA

This is an archive format/library that was intended for game resources
management tool.

Although I could not think of any acronym for this name, it's surely
not Blue Archive the game.

[[_TOC_]]

## CLI

### Usage

```sh
ba h                         # Show help message
ba v                         # Show version info
ba c arc.ba foo/ bar/        # Create archive
ba l arc.ba                  # List of entries in this archive
ba x arc.ba foo/bar/baz.bin  # Extract entries from the archive
```

## Library

- `<ba/ba.h>` - That one header that covers everything this library offers
  (oh also the version).
- `<ba/buffer.h>` - An abstraction over I/O with memory or file.
- `<ba/reader.h>` - Types and functions to read from an archive.
- `<ba/writer.h>` - Types and functions to construct an archive.
- `<ba/ba.hpp>` - `<ba/ba.h>` but with C++ compatibility.
- `<ba/buffer.hpp>` - `<ba/buffer.h>` but with C++ RAII.
- `<ba/reader.hpp>` - `<ba/reader.h>` but with C++ RAII.
- `<ba/writer.hpp>` - `<ba/writer.h>` but with C++ RAII.

## Using

### Build & Install

```sh
cmake -B build -S .
cmake --build build
cmake --install build --prefix=<prefix>
```

### Import for CMake

```cmake
find_package(BA REQUIRED)

target_link_libraries(foo BA::BA)

add_custom_command(OUTPUT "res.ba"
    COMMAND BA::APP "c"
    "${CMAKE_CURRENT_BINARY_DIR}/res.ba"
    "res/"
    DEPENDS BA::APP
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(res DEPENDS "res.ba")
add_dependencies(foo res)
```

## TODO

- [ ] Comment the code
- [ ] Mature the CLI interface
- [ ] Think of a good acronym for BA
