#include <string>

#include "sahara.hpp"

#define declare_case(Name) \
    case Name:             \
        return #Name;

auto to_string(const sahara::Command command) -> const char* {
    switch(command) {
        declare_case(sahara::Command::None);
        declare_case(sahara::Command::Hello);
        declare_case(sahara::Command::HelloResponse);
        declare_case(sahara::Command::ReadData);
        declare_case(sahara::Command::EndTransfer);
        declare_case(sahara::Command::Done);
        declare_case(sahara::Command::DoneResponse);
        declare_case(sahara::Command::Reset);
        declare_case(sahara::Command::ResetResponse);
        declare_case(sahara::Command::MemoryDebug);
        declare_case(sahara::Command::MemoryRead);
        declare_case(sahara::Command::Ready);
        declare_case(sahara::Command::SwitchMode);
        declare_case(sahara::Command::Exec);
        declare_case(sahara::Command::ExecResponse);
        declare_case(sahara::Command::ExecData);
        declare_case(sahara::Command::MemorDebug64);
        declare_case(sahara::Command::MemoryRead64);
        declare_case(sahara::Command::ReadData64);
    default:
        return "?";
    }
}

auto to_string(const sahara::ImageType image_type) -> const char* {
    switch(image_type) {
        declare_case(sahara::ImageType::Binary);
        declare_case(sahara::ImageType::ELF);
    default:
        return "?";
    }
}

auto to_string(const sahara::Status status) -> const char* {
    switch(status) {
        declare_case(sahara::Status::Success);
        declare_case(sahara::Status::InvalidCommand);
        declare_case(sahara::Status::ProtocolMismatch);
        declare_case(sahara::Status::InvalidTargetProtocol);
        declare_case(sahara::Status::InvalidHostProtocol);
        declare_case(sahara::Status::InvalidPacketSize);
        declare_case(sahara::Status::UnexpectedImageID);
        declare_case(sahara::Status::InvalidHeaderSize);
        declare_case(sahara::Status::InvalidDataSize);
        declare_case(sahara::Status::InvalidImageType);
        declare_case(sahara::Status::InvalidTxLength);
        declare_case(sahara::Status::InvalidRxLength);
        declare_case(sahara::Status::GeneralTxRxError);
        declare_case(sahara::Status::ReadDataError);
        declare_case(sahara::Status::UnsupportedNumProgramHeaders);
        declare_case(sahara::Status::InvalidProgramHeaderSize);
        declare_case(sahara::Status::MultipleSharedSegments);
        declare_case(sahara::Status::UnitializedProgramHeaderLocation);
        declare_case(sahara::Status::InvalidDestAddr);
        declare_case(sahara::Status::InvalidImageHeaderDataSize);
        declare_case(sahara::Status::InvalidELFHeader);
        declare_case(sahara::Status::UnknownHostError);
        declare_case(sahara::Status::TimeoutRx);
        declare_case(sahara::Status::TimeoutTx);
        declare_case(sahara::Status::InvalidHostMode);
        declare_case(sahara::Status::InvalidMemoryRead);
        declare_case(sahara::Status::InvalidDataSizeRequest);
        declare_case(sahara::Status::MemoryDebugNotSupported);
        declare_case(sahara::Status::InvalidModeSwitch);
        declare_case(sahara::Status::CommandExecFailure);
        declare_case(sahara::Status::CommandInvalidParam);
        declare_case(sahara::Status::CommandUnsupported);
        declare_case(sahara::Status::InvalidClientCommand);
        declare_case(sahara::Status::HashTableAuthFailure);
        declare_case(sahara::Status::HashTableVerificationFailure);
        declare_case(sahara::Status::HashTableNotFound);
    default:
        return "?";
    }
}

auto to_string(const sahara::Mode mode) -> const char* {
    switch(mode) {
        declare_case(sahara::Mode::ImageTxPending);
        declare_case(sahara::Mode::ImageTxComplete);
        declare_case(sahara::Mode::MemoryDebug);
        declare_case(sahara::Mode::Command);
    default:
        return "?";
    }
}

auto to_string(const sahara::ExecCommand command) -> const char* {
    switch(command) {
        declare_case(sahara::ExecCommand::NOP);
        declare_case(sahara::ExecCommand::ReadSerialNumber);
        declare_case(sahara::ExecCommand::ReadMSMHardwareID);
        declare_case(sahara::ExecCommand::ReadOEMPubKeyHashTable);
        declare_case(sahara::ExecCommand::SwitchDMSS);
        declare_case(sahara::ExecCommand::SwitchStreaming);
        declare_case(sahara::ExecCommand::ReadDebugData);
    default:
        return "?";
    }
}

auto to_string(const uint32_t num) -> std::string {
    return std::to_string(num);
}

#define field_to_string(field)    \
    r += #field;                  \
    r += ": ";                    \
    r += to_string(packet.field); \
    r += "\n";

auto to_string(const sahara::packet::Header& packet) -> std::string {
    auto r = std::string();
    field_to_string(command);
    field_to_string(length);
    return r;
}

auto to_string(const sahara::packet::Hello& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(version);
    field_to_string(supported_version);
    field_to_string(max_packet_size);
    field_to_string(mode);
    return r;
}

auto to_string(const sahara::packet::HelloResponse& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(version);
    field_to_string(supported_version);
    field_to_string(status);
    field_to_string(mode);
    return r;
}

auto to_string(const sahara::packet::ReadData& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(image_id);
    field_to_string(offset);
    field_to_string(size);
    return r;
}

auto to_string(const sahara::packet::ReadData64& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(image_id);
    field_to_string(offset);
    field_to_string(size);
    return r;
}

auto to_string(const sahara::packet::EndTransfer& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(image_id);
    field_to_string(status);
    return r;
}

auto to_string(const sahara::packet::Done& packet) -> std::string {
    return to_string(packet.header);
}

auto to_string(const sahara::packet::DoneResponse& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(image_tx_status);
    return r;
}

auto to_string(const sahara::packet::Reset& packet) -> std::string {
    return to_string(packet.header);
}

auto to_string(const sahara::packet::ResetResponse& packet) -> std::string {
    return to_string(packet.header);
}

auto to_string(const sahara::packet::SwitchMode& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(mode);
    return r;
}

auto to_string(const sahara::packet::Exec& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(command);
    return r;
}

auto to_string(const sahara::packet::ExecResponse& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(client_command);
    field_to_string(data_size);
    return r;
}

auto to_string(const sahara::packet::ExecData& packet) -> std::string {
    auto r = to_string(packet.header);
    field_to_string(command);
    return r;
}

#define try_to_dump(Type) \
    if(size == sizeof(sahara::packet::Type) && header.command == sahara::Command::Type) return to_string(*(std::bit_cast<sahara::packet::Type*>(buf)));

auto try_to_dump_packet(const std::byte* buf, const size_t size) -> std::string {
    const auto& header = *std::bit_cast<sahara::packet::Header*>(buf);
    try_to_dump(Hello);
    try_to_dump(HelloResponse);
    try_to_dump(ReadData);
    try_to_dump(ReadData64);
    try_to_dump(EndTransfer);
    try_to_dump(Done);
    try_to_dump(DoneResponse);
    try_to_dump(Reset);
    try_to_dump(ResetResponse);
    try_to_dump(SwitchMode);
    try_to_dump(Exec);
    try_to_dump(ExecResponse);
    try_to_dump(ExecData);
    return "";
}
