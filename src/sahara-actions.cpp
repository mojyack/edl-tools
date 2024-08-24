#include <unistd.h>

#include "abstract-device.hpp"
#include "macros/unwrap.hpp"
#include "sahara.hpp"
#include "util/file-io.hpp"

namespace {
auto receive_hello(Device& dev) -> bool {
    auto hello = sahara::packet::Hello{};
    ensure(dev.read_struct(&hello, sizeof(hello)), "failed to receive hello command");
    print("version: ", hello.version);
    print("supported version: ", hello.supported_version);
    print("packet size: ", hello.max_packet_size);
    print("mode: ", (int)hello.mode);
    return true;
}

auto get_exec_command_payload(Device& dev, const sahara::ExecCommand command) -> std::optional<std::vector<std::byte>> {
    const auto cmd = sahara::packet::Exec{
        .command = command,
    };
    ensure(dev.write(&cmd, sizeof(cmd)), "failed to send exec command");

    auto res = sahara::packet::ExecResponse{};
    ensure(dev.read_struct(&res, sizeof(res)), "failed to receive exec response");

    const auto payload_req = sahara::packet::ExecData{
        .command = command,
    };
    ensure(dev.write(&payload_req, sizeof(payload_req)), "failed to send exec data command");

    auto payload = std::vector<std::byte>(res.data_size);
    ensure(dev.read_struct(payload.data(), payload.size()), "failed to read payload");
    return payload;
}

auto send_done(Device& dev) -> bool {
    const auto done = sahara::packet::Done{};
    ensure(dev.write(&done, sizeof(done)), "failed to send done command");

    auto res = sahara::packet::DoneResponse{};
    ensure(dev.read_struct(&res, sizeof(res)), "failed to receive done response");
    ensure(res.header.command == sahara::Command::DoneResponse, "unexpected command");

    return true;
}
} // namespace

auto do_command_hello(Device& dev) -> bool {
    ensure(receive_hello(dev));

    const auto res = sahara::packet::HelloResponse{
        .version           = 2,
        .supported_version = 2,
        .status            = sahara::Status::Success,
        .mode              = sahara::Mode::Command,
        .reserved          = {0, 0, 0, 0, 0, 0},
    };
    ensure(dev.write(&res, sizeof(res)), "failed to send hello response");

    auto ready = sahara::packet::ResetResponse{};
    ensure(dev.read(&ready, sizeof(ready)), "failed to receive ready response");
    ensure(ready.header.command == sahara::Command::Ready, "unexpected command");

    return true;
}

auto do_switchmode(Device& dev) -> bool {
    const auto res = sahara::packet::SwitchMode{
        .mode = sahara::Mode::Command,
    };
    ensure(dev.write(&res, sizeof(res)), "failed to send swith mode command");
    return true;
}

auto do_reset(Device& dev) -> bool {
    const auto reset = sahara::packet::Reset{};
    ensure(dev.write(&reset, sizeof(reset)), "failed to send reset command");
    auto res = sahara::packet::ResetResponse{};
    ensure(dev.read(&res, sizeof(res)), "failed to receive reset response");
    ensure(res.header.command == sahara::Command::ResetResponse, "unexpected command");
    return true;
}

auto do_get_serial_number(Device& dev) -> bool {
    unwrap(payload, get_exec_command_payload(dev, sahara::ExecCommand::ReadSerialNumber));

    printf("serial-number: ");
    for(const auto b : payload) {
        printf("%02X", int(b));
    }
    printf("\n");

    return true;
}

auto do_get_msm_hwid(Device& dev) -> bool {
    unwrap(payload, get_exec_command_payload(dev, sahara::ExecCommand::ReadMSMHardwareID));
    ensure(payload.size() >= 8, "hwid payload too short");

    printf("model-id: ");
    for(auto i = 0; i < 2; i += 1) {
        printf("%02X", int(payload[i]));
    }
    printf("\n");
    printf("oem-id: ");
    for(auto i = 2; i < 4; i += 1) {
        printf("%02X", int(payload[i]));
    }
    printf("\n");
    printf("msm-id: ");
    for(auto i = 4; i < 8; i += 1) {
        printf("%02X", int(payload[i]));
    }
    printf("\n");

    return true;
}

auto do_get_pkhash(Device& dev) -> bool {
    unwrap_mut(payload, get_exec_command_payload(dev, sahara::ExecCommand::ReadOEMPubKeyHashTable));

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
        printf("%02X", int(b));
    }
    printf("\n");

    return true;
}

auto do_upload_hello(Device& dev, const char* const programmer_path) -> bool {
    ensure(receive_hello(dev));
    unwrap(programmer, read_file(programmer_path));
    printf("uploading edl programmer, size=%lXbytes\n", programmer.size());

    const auto hello = sahara::packet::HelloResponse{
        .version           = 2,
        .supported_version = 2,
        .status            = sahara::Status::Success,
        .mode              = sahara::Mode::ImageTxPending,
        .reserved          = {0, 0, 0, 0, 0, 0},
    };
    ensure(dev.write(&hello, sizeof(hello)), "failed to send hello response");

    auto buf = std::array<std::byte, sizeof(sahara::packet::ReadData64)>();
    while(true) {
        ensure(dev.read(buf.data(), buf.size()) >= int(sizeof(sahara::packet::Header)), "failed to receive next request");
        const auto& header = *std::bit_cast<sahara::packet::Header*>(buf.data());
        if(header.command == sahara::Command::Done) {
            return send_done(dev);
        }
        if(header.command == sahara::Command::EndTransfer) {
            const auto& packet = *std::bit_cast<sahara::packet::EndTransfer*>(buf.data());
            ensure(packet.status == sahara::Status::Success, "failed to upload programmer");
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
            bail("unexpected command");
        }
        printf("request 0x%lx+0x%lx\n", offset, size);
        if(offset + size < programmer.size()) {
            print("normal upload");
            dev.write(programmer.data() + offset, size);
        } else if(offset >= programmer.size()) {
            print("over");
            auto b = std::vector<std::byte>(size);
            std::fill(b.begin(), b.end(), std::byte(0xff));
            dev.write(b.data(), size);
        } else {
            print("partial");
            auto b = std::vector<std::byte>(size);
            memcpy(b.data(), programmer.data() + offset, programmer.size() - offset);
            std::fill(b.begin() + programmer.size() - offset, b.end(), std::byte(0xff));
            dev.write(b.data(), size);
        }
    }

    return true;
}
