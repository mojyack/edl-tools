#pragma once
#include <cstddef>

namespace buse {
struct Operator {
    size_t file_size;
    size_t total_blocks;
    size_t block_size;

    virtual auto read(void* buf, size_t len, size_t offset) -> int        = 0;
    virtual auto write(const void* buf, size_t len, size_t offset) -> int = 0;
    virtual auto disconnect() -> int                                      = 0;
    virtual auto flush() -> int                                           = 0;
    virtual auto trim(size_t from, size_t len) -> int                     = 0;

    virtual ~Operator() {}
};

auto run(const char* nbd_path, Operator& op) -> int;
} // namespace buse
