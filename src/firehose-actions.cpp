#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "assert.hpp"
#include "firehose-actions.hpp"
#include "util/charconv.hpp"
#include "util/misc.hpp"
#include "xml/xml.hpp"

namespace fh {
namespace {
const auto xml_header = std::string(R"(<?xml version="1.0"?>)");

struct ParsedXML {
    std::string key;
    std::string value;
};

auto parse_xml(std::string_view str) -> std::vector<ParsedXML> {
    auto r = std::vector<ParsedXML>();
    while(!str.empty()) {
        // remove xml header
        const auto header_begin = str.find("<?xml");
        assert_v(header_begin != str.npos, r, "failed to find xml header");
        const auto header_end = str.find("?>", header_begin);
        assert_v(header_end != str.npos, r, "failed to find xml header");
        str.remove_prefix(header_end + 2);

        // find data body
        const auto body_begin = str.find("<");
        assert_v(body_begin != str.npos, r, "failed to find xml body");
        const auto next_header_begin = str.find("<?xml");
        const auto body_end          = next_header_begin != str.npos ? next_header_begin : str.size();
        const auto body              = str.substr(body_begin, body_end - body_begin);
        str.remove_prefix(body_end);

        // parse xml
        const auto node_r = xml::parse(body);
        assert_v(node_r, r, node_r.as_error().cstr());
        const auto& node = node_r.as_value();
        assert_v(node.name == "data", r, "got unknown xml element");
        for(const auto& c : node.children) {
            if(const auto value = c.find_attr("value"); !value) {
                continue;
            } else {
                r.push_back(ParsedXML{std::string(c.name), std::string(*value)});
            }
        }
    }
    return r;
}

auto receive_xml(Device& dev) -> std::vector<ParsedXML> {
    auto       buf       = std::string();
    const auto read_char = [&dev, &buf]() -> bool {
        auto c = char();
        assert_v(dev.read(&c, 1), false);
        buf.push_back(c);
        return true;
    };

    static const auto minimal = std::string_view(R"(<?xml version="1.0" encoding="UTF-8"?><data></data>)");
    buf.resize(minimal.size());
    assert_v(dev.read(buf.data(), minimal.size()), {});
    assert_v(buf.starts_with("<?xml"), {}, "not a xml");

    static const auto marker = std::string_view("</data>");
    while(true) {
        const auto tail = std::string_view(buf.data() + buf.size() - marker.size());
        if(tail == marker) {
            break;
        }
        assert_v(read_char(), {});
    }
    return parse_xml(buf);
}

auto find_response(const std::vector<ParsedXML>& nodes) -> std::string_view {
    for(const auto& r : nodes) {
        if(r.key == "response") {
            return r.value;
        }
    }
    return {};
}

auto wait_for_ack(Device& dev) -> bool {
loop:
    const auto xml = receive_xml(dev);
    assert_v(!xml.empty(), false);
    if(const auto r = find_response(xml); !r.empty()) {
        return r == "ACK";
    }
    goto loop;
}

auto send_rw_command(Device& dev, const size_t disk, const size_t sector_begin, const size_t num_sectors, const char* const command) -> bool {
    const auto node =
        xml::Node{
            .name = "data",
        }
            .append_children({
                xml::Node{.name = command}
                    .append_attrs({
                        {"SECTOR_SIZE_IN_BYTES", std::to_string(fh::bytes_per_sector)},
                        {"num_partition_sectors", std::to_string(num_sectors)},
                        {"physical_partition_number", std::to_string(disk)},
                        {"start_sector", std::to_string(sector_begin)},
                    }),
            });
    const auto payload = xml_header + xml::deparse(node);
    assert_v(dev.write(payload.data(), payload.size()), false, "failed to send ", command);
    assert_v(wait_for_ack(dev), false, "cannot read ready ack");
    return true;
}

struct RWArgs {
    size_t           disk;
    size_t           sector_begin;
    size_t           num_sectors;
    std::string_view file;
};

auto parse_rw_args(const std::string_view str, RWArgs& args) -> bool {
    const auto elms = split(str, " ");
    assert_v(elms.size() == 4, false, "invalid number of arguments");
    const auto disk = from_chars<size_t>(elms[0]);
    assert_v(disk, false, "invalid disk");
    const auto sector_begin = from_chars<size_t>(elms[1]);
    assert_v(sector_begin, false, "invalid sector begin");
    const auto num_sectors = from_chars<size_t>(elms[2]);
    assert_v(num_sectors, false, "invalid num sectors");
    const auto output_name = elms[3];

    args = RWArgs{*disk, *sector_begin, *num_sectors, output_name};
    return true;
}
} // namespace

auto send_nop(Device& dev) -> bool {
    const auto node =
        xml::Node{
            .name = "data",
        }
            .append_children({
                xml::Node{.name = "nop "},
            });
    const auto payload = xml_header + xml::deparse(node);
    assert_v(dev.write(payload.data(), payload.size()), false, "failed to send command");

    auto logs = std::vector<ParsedXML>();
    while(true) {
        auto nodes = receive_xml(dev);
        for(auto& node : nodes) {
            if(node.key == "response") {
                goto end;
            }
            if(node.key == "log") {
                if(node.value.starts_with("End of supported functions") ||
                   node.value.starts_with("ERROR")) {
                    goto end;
                }
            }
            logs.emplace_back(std::move(node));
        }
    }
end:
    auto supported_features_begin = false;
    auto supported_features       = std::vector<std::string_view>();
    for(const auto& log : logs) {
        if(supported_features_begin) {
            supported_features.emplace_back(log.value);
        }
        if(log.value.starts_with("Chip serial num")) {
            print(log.value);
        } else if(log.value.starts_with("Supported Functions")) {
            supported_features_begin = true;
        } else if(log.value.starts_with("End of supported functions")) {
            supported_features_begin = false;
        }
    }
    print("supported features: ");
    for(const auto& f : supported_features) {
        print("    ", f);
    }
    return true;
}

auto send_configure(Device& dev) -> bool {
    const auto node =
        xml::Node{
            .name = "data",
        }
            .append_children({
                xml::Node{.name = "configure"}
                    .append_attrs({
                        {"MemoryName", "UFS"},
                        {"Verbose", "1"},
                        {"AlwaysValidate", "0"},
                        {"MaxDigestTableSizeInBytes", "2048"},
                        {"MaxPayloadSizeToTargetInBytes", "4096"},
                        {"ZLPAwareHost", "1"},
                        {"SkipStorageInit", "0"},
                        {"SkipWrite", "0"},
                    }),
            });
    const auto payload = xml_header + xml::deparse(node);
    assert_v(dev.write(payload.data(), payload.size()), false, "failed to send command");
    assert_v(wait_for_ack(dev), false, "cannot read done ack");
    return true;
}

auto send_reset(Device& dev) -> bool {
    const auto node =
        xml::Node{
            .name = "data",
        }
            .append_children({
                xml::Node{.name = "power"}
                    .append_attrs({
                        {"value", "reset"},
                    }),
            });
    const auto payload = xml_header + xml::deparse(node);
    assert_v(dev.write(payload.data(), payload.size()), false, "failed to send command");
    print("reset done");
    // halt
    auto buf = std::array<char, 4096>();
    while(true) {
        assert_v(dev.read(buf.data(), buf.size()), false);
    }
    exit(0);
}

auto read_disk(Device& dev, const size_t disk, const size_t sector_begin, const size_t num_sectors, std::byte* const output_buffer) -> bool {
    assert_v(send_rw_command(dev, disk, sector_begin, num_sectors, "read"), false);
    print("read ready");

    auto bytes_left = num_sectors * bytes_per_sector;
    auto ack_str    = std::string_view(); // sometimes ack is included in data packet
    auto buf        = std::array<std::byte, 4096>();
    while(bytes_left > 0) {
        const auto size = dev.read(buf.data(), buf.size());
        assert_v(size > 0, false, "failed to receive dump data");
        const auto bytes_to_read = std::min(size_t(size), bytes_left);
        memcpy(output_buffer + num_sectors * bytes_per_sector - bytes_left, buf.data(), bytes_to_read);
        if(size_t(size) > bytes_left) {
            ack_str = std::string_view(std::bit_cast<char*>(buf.data()) + bytes_left, size - bytes_to_read);
        }
        bytes_left -= bytes_to_read;
        printf("%d bytes received, remain %lu bytes\n", size, bytes_left);
    }

    if(!ack_str.empty()) {
        if(const auto r = find_response(parse_xml(ack_str)); !r.empty()) {
            assert_v(r == "ACK", false, "cannot read done ack");
            goto end;
        }
    }

    assert_v(wait_for_ack(dev), false, "cannot read done ack");

end:
    print("read done");
    return true;
}

auto read_to_file(Device& dev, const std::string_view args_str) -> bool {
    auto args = RWArgs();
    assert_v(parse_rw_args(args_str, args), false);

    const auto total_bytes = args.num_sectors * fh::bytes_per_sector;
    const auto output_fd   = open(std::string(args.file).data(), O_RDWR | O_CREAT, 0644);
    assert_v(output_fd >= 0, false);
    assert_v(ftruncate(output_fd, total_bytes) == 0, false);
    const auto output_buf = mmap(NULL, total_bytes, PROT_WRITE, MAP_SHARED, output_fd, 0);
    assert_v(output_buf != MAP_FAILED, false);

    read_disk(dev, args.disk, args.sector_begin, args.num_sectors, std::bit_cast<std::byte*>(output_buf));

    assert_v(close(output_fd) == 0, false);
    assert_v(munmap(output_buf, total_bytes) == 0, false);
    return true;
}

auto write_disk(Device& dev, const size_t disk, const size_t sector_begin, const size_t num_sectors, const std::byte* input_buffer) -> bool {
    assert_v(send_rw_command(dev, disk, sector_begin, num_sectors, "program"), false);
    print("write ready");

    auto bytes_left = num_sectors * bytes_per_sector;
    while(bytes_left > 0) {
        const auto bytes_to_write = size_t(bytes_per_sector);
        assert_v(dev.write(input_buffer, bytes_to_write), false, "failed to write data: ", strerror(errno));
        input_buffer += bytes_to_write;
        bytes_left -= bytes_to_write;
        printf("%lu bytes sent, remain %lu bytes\n", bytes_to_write, bytes_left);
    }

    // quirk: device does not respond until the next packet arrived
    // send dummy input and consume some error responses
    // assert_v(wait_for_ack(dev), false, "cannot read done ack");
    auto dummy = char(' ');
    dev.write(&dummy, 1);
    auto step = 0;
    while(step < 3) {
        for(const auto& node : receive_xml(dev)) {
            if(step == 0) {
                if(node.key == "response") {
                    assert_v(node.value == "ACK", false, "cannot read done ack");
                    step = 1;
                }
            } else {
                if(node.key == "log" && node.value.starts_with("ERROR")) {
                    // this packet is caused by the dummy input
                    step += 1;
                }
            }
        }
    }

    print("write done");
    return true;
}

auto write_from_file(Device& dev, std::string_view args_str) -> bool {
    auto args = RWArgs();
    assert_v(parse_rw_args(args_str, args), false);

    const auto total_bytes = args.num_sectors * fh::bytes_per_sector;
    const auto input_fd    = open(std::string(args.file).data(), O_RDONLY);
    assert_v(input_fd >= 0, false);
    const auto input_buf = mmap(NULL, total_bytes, PROT_READ, MAP_PRIVATE, input_fd, 0);
    assert_v(input_buf != MAP_FAILED, false);

    write_disk(dev, args.disk, args.sector_begin, args.num_sectors, std::bit_cast<std::byte*>(input_buf));

    assert_v(close(input_fd) == 0, false);
    assert_v(munmap(input_buf, total_bytes) == 0, false);
    return true;
}
} // namespace fh
