#include <cstring>
#include <vector>

#include <errno.h>

#include "block-operator.hpp"

namespace buse {
namespace {
template <class T, class U>
auto align(const T value, const U unit) -> T {
    return (value / unit) * unit;
}

auto generic_io(BlockOperator& self, const bool write, const size_t offset, const size_t len, void* const buf) -> int {
    if(len % self.block_size == 0 && offset % self.block_size == 0) {
        const auto blocks = len / self.block_size;
        const auto block  = offset / self.block_size;
        return write ? self.write_block(block, blocks, buf) : self.read_block(block, blocks, buf);
    }

    const auto aligned_offset = align(offset, self.block_size);
    const auto offset_gap     = offset - aligned_offset;
    const auto block          = aligned_offset / self.block_size;
    const auto blocks         = ((len + offset_gap) + self.block_size - 1) / self.block_size;
    auto       tmp            = std::vector<std::byte>(blocks * self.block_size);
    if(write) {
        if(offset_gap != 0 && !self.read_block(block, 1, tmp.data())) {
            return EIO;
        }
        const auto last_block = block + blocks - 1;
        if((len - (self.block_size - offset_gap)) % self.block_size != 0 &&
           !self.read_block(last_block, 1, tmp.data() + (blocks - 1) * self.block_size)) {
            return EIO;
        }
        memcpy(tmp.data() + offset_gap, buf, len);
        if(!self.write_block(block, blocks, tmp.data())) {
            return EIO;
        }
        return 0;
    } else {
        if(!self.read_block(block, blocks, tmp.data())) {
            return EIO;
        }
        memcpy(buf, tmp.data() + offset_gap, len);
        return 0;
    }
}
} // namespace

auto BlockOperator::read(size_t offset, size_t len, void* buf) -> int {
    return generic_io(*this, false, offset, len, buf);
}

auto BlockOperator::write(size_t offset, size_t len, const void* buf) -> int {
    return generic_io(*this, true, offset, len, const_cast<void*>(buf));
}
} // namespace buse
