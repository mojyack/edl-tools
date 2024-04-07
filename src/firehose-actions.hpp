#pragma once
#include <string_view>

#include "abstract-device.hpp"

namespace fh {
constexpr auto bytes_per_sector = 0x1000;

// TODO
// implement getstorageinfo
// <?xml version="1.0"?><data><getstorageinfo /></data>
// <?xml version="1.0"?><data><getstorageinfo physical_partition_number="1"/></data>

auto send_nop(Device& dev) -> bool;
auto send_configure(Device& dev) -> bool;
auto send_reset(Device& dev) -> bool;
auto read_disk(Device& dev, size_t disk, size_t sector_begin, size_t num_sectors, std::byte* output_buffer) -> bool;
auto read_to_file(Device& dev, std::string_view args) -> bool;
auto write_disk(Device& dev, size_t disk, size_t sector_begin, size_t num_sectors, const std::byte* input_buffer) -> bool;
auto write_from_file(Device& dev, std::string_view args) -> bool;
} // namespace fh
