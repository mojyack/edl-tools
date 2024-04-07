#include <vector>

#include "assert.hpp"
#include "firehose-actions.hpp"
#include "gpt-parser.hpp"
#include "gpt.hpp"

auto parse_gpt(Device& dev, const size_t disk) -> GPTParseResult {
    auto buffer = std::vector<std::byte>(fh::bytes_per_sector);

    assert_v(fh::read_disk(dev, disk, 0, 1, buffer.data()), {}, "failed to dump mbr");
    const auto& mbr = *std::bit_cast<gpt::MBR*>(buffer.data());
    assert_v(mbr.signature[0] == 0x55 && mbr.signature[1] == 0xAA, {}, "invalid mbr signature");
    assert_v(mbr.partition[0].type == 0xEE, {}, "legacy format disk");

    assert_v(fh::read_disk(dev, disk, 1, 1, buffer.data()), {}, "failed to dump efi table");
    const auto header = *std::bit_cast<gpt::PartitionTableHeader*>(buffer.data());
    assert_v(std::string_view(header.signature, 8) == "EFI PART", {}, "invalid gpt signature");
    assert_v(header.entry_size == 128, {}, "not implemented");

    auto partitions = std::vector<Partition>();
    auto buffer_lba = 1u;
    for(auto i = 0u; i < header.num_entries; i += 1) {
        const auto lba = header.entry_array_lba + sizeof(gpt::PartitionEntry) * i / fh::bytes_per_sector;
        if(lba != buffer_lba) {
            assert_v(fh::read_disk(dev, disk, lba, 1, buffer.data()), {}, "failed to dump partition entry");
            buffer_lba = lba;
        }
        const auto& entry = *std::bit_cast<gpt::PartitionEntry*>(buffer.data() + sizeof(gpt::PartitionEntry) * i % fh::bytes_per_sector);
        if(entry.type == gpt::GUID{0, 0, 0, {0}}) {
            continue;
        }
        auto label = std::string();
        for(auto i = 0; i < 18 && entry.name[i] != '\n'; i += 1) {
            label.push_back(entry.name[i]);
        }
        partitions.push_back(Partition{entry.lba_start, entry.lba_last, std::move(label)});
    }
    return GPTParseResult{
        .usable_first   = header.first_usable,
        .usable_last    = header.last_usable,
        .header_lba     = header.lba_self,
        .header_alt_lba = header.lba_alt,
        .partitions     = std::move(partitions),
    };
}
