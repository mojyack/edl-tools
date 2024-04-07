#include <unistd.h>

#include "abstract-device.hpp"
#include "assert.hpp"
#include "sahara.hpp"
#include "util/misc.hpp"

namespace {
#define unwrap_ob(var, opt, ...) \
    const auto var##_o = opt;    \
    if(!var##_o) {               \
        print(__VA_ARGS__);      \
        return false;            \
    }                            \
    const auto& var = var##_o.as_value();

auto receive_hello(Device& dev) -> bool {
    auto hello = sahara::packet::Hello{};
    assert_v(dev.read_struct(&hello, sizeof(hello)), false, "failed to receive hello command");
    print("version: ", hello.version);
    print("supported version: ", hello.supported_version);
    print("packet size: ", hello.max_packet_size);
    print("mode: ", (int)hello.mode);
    return true;
}

auto get_exec_command_payload(Device& dev, const sahara::ExecCommand command) -> std::vector<std::uint8_t> {
    const auto cmd = sahara::packet::Exec{
        .command = command,
    };
    assert_v(dev.write(&cmd, sizeof(cmd)), {}, "failed to send exec command");

    auto res = sahara::packet::ExecResponse{};
    assert_v(dev.read_struct(&res, sizeof(res)), {}, "failed to receive exec response");

    const auto payload_req = sahara::packet::ExecData{
        .command = command,
    };
    assert_v(dev.write(&payload_req, sizeof(payload_req)), {}, "failed to send exec data command");

    auto payload = std::vector<std::uint8_t>(res.data_size);
    assert_v(dev.read_struct(payload.data(), payload.size()), {}, "failed to read payload");
    return payload;
}

auto send_done(Device& dev) -> bool {
    const auto done = sahara::packet::Done{};
    assert_v(dev.write(&done, sizeof(done)), false, "failed to send done command");

    auto res = sahara::packet::DoneResponse{};
    assert_v(dev.read_struct(&res, sizeof(res)), false, "failed to receive done response");
    assert_v(res.header.command == sahara::Command::DoneResponse, false, "unexpected command");

    return true;
}
} // namespace

auto do_command_hello(Device& dev) -> bool {
    assert_v(receive_hello(dev), false);

    const auto res = sahara::packet::HelloResponse{
        .version           = 2,
        .supported_version = 2,
        .status            = sahara::Status::Success,
        .mode              = sahara::Mode::Command,
        .reserved          = {0, 0, 0, 0, 0, 0},
    };
    assert_v(dev.write(&res, sizeof(res)), false, "failed to send hello response");

    auto ready = sahara::packet::ResetResponse{};
    assert_v(dev.read(&ready, sizeof(ready)), {}, "failed to receive ready response");
    assert_v(ready.header.command == sahara::Command::Ready, false, "unexpected command");

    return true;
}

auto do_switchmode(Device& dev) -> bool {
    const auto res = sahara::packet::SwitchMode{
        .mode = sahara::Mode::Command,
    };
    assert_v(dev.write(&res, sizeof(res)), false, "failed to send swith mode command");
    return true;
}

auto do_reset(Device& dev) -> bool {
    const auto reset = sahara::packet::Reset{};
    assert_v(dev.write(&reset, sizeof(reset)), false, "failed to send reset command");
    auto res = sahara::packet::ResetResponse{};
    assert_v(dev.read(&res, sizeof(res)), false, "failed to receive reset response");
    assert_v(res.header.command == sahara::Command::ResetResponse, false, "unexpected command");
    return true;
}

auto do_get_serial_number(Device& dev) -> bool {
    const auto payload = get_exec_command_payload(dev, sahara::ExecCommand::ReadSerialNumber);
    assert_v(!payload.empty(), false);

    printf("serial-number: ");
    for(const auto b : payload) {
        printf("%02X", b);
    }
    printf("\n");

    return true;
}

auto do_get_msm_hwid(Device& dev) -> bool {
    const auto payload = get_exec_command_payload(dev, sahara::ExecCommand::ReadMSMHardwareID);
    assert_v(payload.size() >= 8, false, "hwid payload too short");

    printf("model-id: ");
    for(auto i = 0; i < 2; i += 1) {
        printf("%02X", payload[i]);
    }
    printf("\n");
    printf("oem-id: ");
    for(auto i = 2; i < 4; i += 1) {
        printf("%02X", payload[i]);
    }
    printf("\n");
    printf("msm-id: ");
    for(auto i = 4; i < 8; i += 1) {
        printf("%02X", payload[i]);
    }
    printf("\n");

    return true;
}

auto do_get_pkhash(Device& dev) -> bool {
    auto payload = get_exec_command_payload(dev, sahara::ExecCommand::ReadOEMPubKeyHashTable);
    assert_v(payload.size() >= 4, false, "hwid payload too short");

    // remove duplicated part
    const auto head = *std::bit_cast<uint32_t*>(payload.data());
    for(auto i = 4u; i + 4 <= payload.size(); i += 4) {
        const auto block = *std::bit_cast<uint32_t*>(payload.data() + i);
        if(block == head) {
            payload.resize(i);
        }
    }
    printf("pkhash: ");
    for(const auto b : payload) {
        printf("%02X", b);
    }
    printf("\n");

    return true;
}

auto do_upload_hello(Device& dev, const std::string_view programmer) -> bool {
    assert_v(receive_hello(dev), false);
    unwrap_ob(bin, read_binary(programmer));
    printf("uploading edl programmer, size=%lXbytes\n", bin.size());

    const auto hello = sahara::packet::HelloResponse{
        .version           = 2,
        .supported_version = 2,
        .status            = sahara::Status::Success,
        .mode              = sahara::Mode::ImageTxPending,
        .reserved          = {0, 0, 0, 0, 0, 0},
    };
    assert_v(dev.write(&hello, sizeof(hello)), false, "failed to send hello response");

    auto buf = std::array<std::byte, sizeof(sahara::packet::ReadData64)>();
    while(true) {
        assert_v(dev.read(buf.data(), buf.size()) >= int(sizeof(sahara::packet::Header)), false, "failed to receive next request");
        const auto& header = *std::bit_cast<sahara::packet::Header*>(buf.data());
        if(header.command == sahara::Command::Done) {
            return send_done(dev);
        }
        if(header.command == sahara::Command::EndTransfer) {
            const auto& packet = *std::bit_cast<sahara::packet::EndTransfer*>(buf.data());
            assert_v(packet.status == sahara::Status::Success, "failed to upload programmer");
            return send_done(dev);
        }
        auto offset = uint64_t(0);
        auto size   = uint64_t(0);
        if(header.command == sahara::Command::ReadData) {
            const auto& packet = *std::bit_cast<sahara::packet::ReadData*>(buf.data());
            offset             = packet.offset;
            size               = packet.size;
        } else if(header.command == sahara::Command::ReadData64) {
            const auto& packet = *std::bit_cast<sahara::packet::ReadData64*>(buf.data());
            offset             = packet.offset;
            size               = packet.size;
        } else {
            print("unexpected command");
            return false;
        }
        printf("request 0x%lx+0x%lx\n", offset, size);
        if(offset + size < bin.size()) {
            dev.write(bin.data() + offset, size);
        } else if(offset >= bin.size()) {
            auto b = std::vector<std::byte>(size);
            std::fill(b.begin(), b.end(), std::byte(0xff));
            dev.write(b.data(), size);
        } else {
            auto b = std::vector<std::byte>(size);
            memcpy(b.data(), bin.data() + offset, bin.size() - offset);
            std::fill(b.begin() + bin.size() - offset, b.end(), std::byte(0xff));
            dev.write(b.data(), size);
        }
    }

    return true;
}
