// this file is just a document
#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace fht {
constexpr static auto bytes_per_sector = 16;

auto test_buf = (char*)(nullptr);

auto write_disk(Device& dev, const size_t disk, const size_t sector_begin, const size_t num_sectors, const std::byte* input_buffer) -> bool {
    memcpy(test_buf + sector_begin * 16, input_buffer, num_sectors * 16);
    return true;
}
auto read_disk(Device& dev, const size_t disk, const size_t sector_begin, const size_t num_sectors, std::byte* output_buffer) -> bool {
    memcpy(output_buffer, test_buf + sector_begin * 16, num_sectors * 16);
    return true;
}
}

auto test() -> bool {
    const auto total_bytes = fht::bytes_per_sector * fht::bytes_per_sector;
    const auto output_fd   = open("test.txt", O_RDWR | O_CREAT, 0644);
    assert_v(output_fd >= 0, false);
    assert_v(ftruncate(output_fd, total_bytes) == 0, false);
    const auto output_buf = (char*)mmap(NULL, total_bytes, PROT_WRITE, MAP_SHARED, output_fd, 0);
    assert_v(output_buf != MAP_FAILED, false);
    for(auto i = 0; i < fht::bytes_per_sector; i += 1) {
        for(auto j = 0; j < fht::bytes_per_sector; j += 1) {
            output_buf[i * fht::bytes_per_sector + j] = 'A' + i;
        }
    }

    fht::test_buf = output_buf;

    auto op       = EDLOperator{};
    op.block_size = fht::bytes_per_sector;
    print("1");
    op.write("1111111111111111", 16, 0);
    print("2");
    op.write("2222222222222222", 16, 24);
    print("3");
    op.write("33333333333333333333333333333333", 32, 40);
    print("4");
    op.write("44444444", 8, 0x61);
    print("5");
    op.write("55555555555555555", 17, 0x70);
    print("6");
    op.write("66666666666666666", 17, 0x8f);
    print("7");
    op.write("7777777777777777777777777777777777", 33, 0xaf);
    print("8");
    op.write("8888888888888888888888888888888888", 33, 0xd0);
    return true;
}
