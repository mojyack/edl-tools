#include <array>

#include "assert.hpp"
#include "buse/block-operator.hpp"
#include "buse/buse.hpp"
#include "firehose-actions.hpp"
#include "serial-device.hpp"
#include "util/charconv.hpp"

namespace {
struct EDLOperator : buse::BlockOperator {
    Device* dev;
    int     disk;

    auto read_block(const size_t block, const size_t blocks, void* buf) -> int override {
        return fh::read_disk(*dev, disk, block, blocks, std::bit_cast<std::byte*>(buf)) ? 0 : EIO;
    }

    auto write_block(size_t block, size_t blocks, const void* buf) -> int override {
        return fh::write_disk(*dev, disk, block, blocks, std::bit_cast<std::byte*>(buf)) ? 0 : EIO;
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

auto assume_total_blocks(Device& dev, const size_t disk) -> size_t {
    auto current  = size_t(1024) * 1024 * 4 / fh::bytes_per_sector; // 4MiB
    auto null_buf = std::array<std::byte, fh::bytes_per_sector>();
    while(true) {
        if(fh::read_disk(dev, disk, current, 1, null_buf.data())) {
            current *= 2;
        } else {
            break;
        }
    }
    current /= 2;
    auto step = current / 2;
    while(true) {
        if(fh::read_disk(dev, disk, current, 1, null_buf.data())) {
            if(step == 0) {
                return current + 1;
            }
            current += step;
        } else {
            if(step == 0) {
                return current;
            }
            current -= step;
        }
        step /= 2;
    }
}
} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    assert_v(argc == 3, 1, "argc != 3");
    auto dev = setup_serial_device(argv[1]);
    assert_v(dev != nullptr, 1);

    const auto disk_o = from_chars<size_t>(argv[2]);
    assert_v(disk_o, 1, "invalid disk number");
    const auto disk = *disk_o;

    const auto last_lba = assume_total_blocks(*dev, disk);
    printf("total size = %lu blocks %lu KiB %lu MiB\n", last_lba, last_lba * 4, last_lba * 4 / 1024);

    return run_edl_abuse(*dev, disk, last_lba);
}
