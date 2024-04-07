#include <vector>

#include "assert.hpp"
#include "buse/buse.hpp"
#include "firehose-actions.hpp"
#include "gpt-parser.hpp"
#include "serial-device.hpp"
#include "util/charconv.hpp"

namespace {
template <class T, class U>
auto align(const T value, const U unit) -> T {
    return (value / unit) * unit;
}

struct EDLOperator : buse::Operator {
    Device* dev;
    int     disk;

    auto aligned_io(const bool write, void* const buf, const size_t len, const size_t offset) -> int {
        const auto block  = offset / block_size;
        const auto blocks = len / block_size;

        auto r = true;
        if(write) {
            r = fh::write_disk(*dev, disk, block, blocks, std::bit_cast<std::byte*>(buf));
        } else {
            r = fh::read_disk(*dev, disk, block, blocks, std::bit_cast<std::byte*>(buf));
        }
        return r ? 0 : EIO;
    }

    // a a a a b b b b c c c c
    // 0 1 2 3 4 5 6 7 8 9 a b
    // 7 3           I____
    //         I
    // 4 5     I________
    //         I
    auto generic_io(const bool write, void* const buf, const size_t len, const size_t offset) -> int {
        if(len % block_size == 0 && offset % block_size == 0) {
            return aligned_io(write, buf, len, offset);
        } else {
            const auto aligned_offset = align(offset, block_size);
            const auto offset_gap     = offset - aligned_offset;
            const auto block          = aligned_offset / block_size;
            const auto blocks         = ((len + offset_gap) + block_size - 1) / block_size;
            auto       tmp            = std::vector<std::byte>(blocks * block_size);
            if(write) {
                if(offset_gap != 0 && !fh::read_disk(*dev, disk, block, 1, tmp.data())) {
                    return EIO;
                }
                const auto last_block = block + blocks - 1;
                if((len - (block_size - offset_gap)) % block_size != 0 &&
                   !fh::read_disk(*dev, disk, last_block, 1, tmp.data() + (blocks - 1) * block_size)) {
                    return EIO;
                }
                memcpy(tmp.data() + offset_gap, buf, len);
                if(!fh::write_disk(*dev, disk, block, blocks, tmp.data())) {
                    return EIO;
                }
                return 0;
            } else {
                if(!fh::read_disk(*dev, disk, block, blocks, tmp.data())) {
                    return EIO;
                }
                memcpy(buf, tmp.data() + offset_gap, len);
                return 0;
            }
        }
    }

    auto read(void* buf, const size_t len, const size_t offset) -> int override {
        return generic_io(false, buf, len, offset);
    }

    auto write(const void* buf, size_t len, size_t offset) -> int override {
        return generic_io(true, const_cast<void*>(buf), len, offset);
    }

    auto disconnect() -> int override {
        return 0;
    }

    auto flush() -> int override {
        return 0;
    }

    auto trim(size_t /*from*/, size_t /*len*/) -> int override {
        return 0;
    }
};

auto run_edl_abuse(Device& dev, const size_t disk, const size_t total_blocks) -> int {
    auto op         = EDLOperator{};
    op.dev          = &dev;
    op.disk         = disk;
    op.file_size    = total_blocks * fh::bytes_per_sector;
    op.block_size   = fh::bytes_per_sector;
    op.total_blocks = total_blocks;
    return buse::run("/dev/nbd0", op);
}
} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    assert_v(argc == 3, 1, "argc != 3");
    auto dev = setup_serial_device(argv[1]);
    assert_v(dev != nullptr, 1);

    const auto disk_o = from_chars<size_t>(argv[2]);
    assert_v(disk_o, 1, "invalid disk number");
    const auto disk = *disk_o;

    const auto parsed = parse_gpt(*dev, disk);
    for(auto p : parsed.partitions) {
        auto size = p.last_lba - p.start_lba;
        printf("    %s %lu ~ %lu (%lu KiB)\n", p.label.data(), p.start_lba, p.last_lba, size * 4);
    }
    const auto last_lba = parsed.header_alt_lba != 0 ? parsed.header_alt_lba + 1 : parsed.usable_last;
    printf("total size = %lu KiB %lu MiB\n", last_lba * 4, last_lba * 4 / 1024);

    return run_edl_abuse(*dev, disk, last_lba);
}
