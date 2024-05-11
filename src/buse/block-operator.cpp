#include <vector>

#include "block-operator.hpp"

namespace buse {
namespace {
template <class T, class U>
auto align(const T value, const U unit) -> T {
    return (value / unit) * unit;
}

auto generic_io(BlockOperator& self, const bool write, void* const buf, const size_t len, const size_t offset) -> int {
    if(len % self.block_size == 0 && offset % self.block_size == 0) {
        const auto blocks = len / self.block_size;
        const auto block  = offset / self.block_size;
        return write ? self.write_block(buf, blocks, block) : self.read_block(buf, blocks, block);
    }

    const auto aligned_offset = align(offset, self.block_size);
    const auto offset_gap     = offset - aligned_offset;
    const auto block          = aligned_offset / self.block_size;
    const auto blocks         = ((len + offset_gap) + self.block_size - 1) / self.block_size;
    auto       tmp            = std::vector<std::byte>(blocks * self.block_size);
    if(write) {
        if(offset_gap != 0 && !self.read_block(tmp.data(), 1, block)) {
            return EIO;
        }
        const auto last_block = block + blocks - 1;
        if((len - (self.block_size - offset_gap)) % self.block_size != 0 &&
           !self.read_block(tmp.data() + (blocks - 1) * self.block_size, 1, last_block)) {
            return EIO;
        }
        memcpy(tmp.data() + offset_gap, buf, len);
        if(!self.write_block(tmp.data(), blocks, block)) {
            return EIO;
        }
        return 0;
    } else {
        if(!self.read_block(tmp.data(), blocks, block)) {
            return EIO;
        }
        memcpy(buf, tmp.data() + offset_gap, len);
        return 0;
    }
}
} // namespace

auto BlockOperator::read(void* buf, size_t len, size_t offset) -> int {
    return generic_io(*this, false, buf, len, offset);
}

auto BlockOperator::write(const void* buf, size_t len, size_t offset) -> int {
    return generic_io(*this, true, const_cast<void*>(buf), len, offset);
}
} // namespace buse
