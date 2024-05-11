#pragma once
#include <cstddef>

namespace buse {
struct Operator {
    size_t file_size;
    size_t total_blocks;
    size_t block_size;

    virtual auto read(size_t offset, size_t len, void* buf) -> int        = 0;
    virtual auto write(size_t offset, size_t len, const void* buf) -> int = 0;
    virtual auto disconnect() -> int                                      = 0;
    virtual auto flush() -> int                                           = 0;
    virtual auto trim(size_t from, size_t len) -> int                     = 0;

    virtual ~Operator() {}
};
} // namespace buse
