#pragma once
#include <string>

namespace gpt {
struct MBR {
    struct PartitionTable {
        uint8_t bootable;
        uint8_t first_sector[3];
        uint8_t type;
        uint8_t last_sector[3];
        uint8_t first_lba_sector[4];
        uint8_t num_sectors[4];
    } __attribute__((packed));

    uint8_t        loader[446];
    PartitionTable partition[4];
    uint8_t        signature[2];
} __attribute__((packed));

static_assert(sizeof(MBR) == 512);

struct GUID {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];

    auto to_string() const -> std::string {
        auto buf = std::string();
        buf.resize(32 + 4 + 1);
        sprintf(buf.data(), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", data1, data2, data3, data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7]);
        return buf;
    }

    auto operator==(const GUID& o) const -> bool {
        return data1 == o.data1 && data2 == o.data2 && data3 == o.data3 && memcmp(data4, o.data4, 8) == 0;
    }
} __attribute__((packed));

struct PartitionTableHeader {
    char     signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t gpt_header_checksum;
    uint32_t reserved1;
    uint64_t lba_self;
    uint64_t lba_alt;
    uint64_t first_usable;
    uint64_t last_usable;
    GUID     disk_guid;
    uint64_t entry_array_lba;
    uint32_t num_entries;
    uint32_t entry_size;
    uint32_t entry_array_checksum;
} __attribute__((packed));

struct PartitionEntry {
    GUID     type;
    GUID     id;
    uint64_t lba_start;
    uint64_t lba_last;
    uint64_t attribute;
    char16_t name[36];
} __attribute__((packed));
} // namespace gpt
