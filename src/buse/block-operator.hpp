#pragma once
#include "operator.hpp"

namespace buse {
struct BlockOperator : Operator {
    auto read(size_t offset, size_t len, void* buf) -> int override;
    auto write(size_t offset, size_t len, const void* buf) -> int override;

    virtual auto read_block(size_t block, size_t blocks, void* buf) -> int        = 0;
    virtual auto write_block(size_t block, size_t blocks, const void* buf) -> int = 0;

    virtual ~BlockOperator() {}
};
} // namespace buse
