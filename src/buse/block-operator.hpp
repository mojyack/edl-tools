#pragma once
#include "operator.hpp"

namespace buse {
struct BlockOperator : Operator {
    auto read(void* buf, size_t len, size_t offset) -> int override;
    auto write(const void* buf, size_t len, size_t offset) -> int override;

    virtual auto read_block(void* buf, size_t blocks, size_t block) -> int     = 0;
    virtual auto write_block(const void* buf, size_t blocks, size_t block) -> int = 0;

    virtual ~BlockOperator() {}
};
} // namespace buse
