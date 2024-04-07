#pragma once
#include <string>
#include <vector>

#include "abstract-device.hpp"

struct Partition {
    size_t      start_lba;
    size_t      last_lba;
    std::string label;
};

struct GPTParseResult {
    size_t                 usable_first;
    size_t                 usable_last;
    size_t                 header_lba;
    size_t                 header_alt_lba;
    std::vector<Partition> partitions;
};

auto parse_gpt(Device& dev, const size_t disk) -> GPTParseResult;
