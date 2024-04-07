#pragma once
#include <cstdint>

namespace sahara {
constexpr auto version           = 2;
constexpr auto supported_version = 4;
constexpr auto buffer_size       = 4 * 1024; // max 1024 * 1024

enum class Command : uint32_t {
    None          = 0x00,
    Hello         = 0x01, // ->
    HelloResponse = 0x02, // <-
    ReadData      = 0x03, // ->
    EndTransfer   = 0x04, // ->
    Done          = 0x05, // <-
    DoneResponse  = 0x06, // ->
    Reset         = 0x07, // <-
    ResetResponse = 0x08, // ->
    MemoryDebug   = 0x09, // ->
    MemoryRead    = 0x0A, // <-
    Ready         = 0x0B, // ->
    SwitchMode    = 0x0C, // <-
    Exec          = 0x0D, // <-
    ExecResponse  = 0x0E, // ->
    ExecData      = 0x0F, // <-
    MemorDebug64  = 0x10, // ->
    MemoryRead64  = 0x11, // <-
    ReadData64    = 0x12,
    Limit,
};

enum class ImageType : uint32_t {
    Binary = 0x00,
    ELF    = 0x01,
};

enum class Status : uint32_t {
    Success                          = 0x00,
    InvalidCommand                   = 0x01,
    ProtocolMismatch                 = 0x02,
    InvalidTargetProtocol            = 0x03,
    InvalidHostProtocol              = 0x04,
    InvalidPacketSize                = 0x05,
    UnexpectedImageID                = 0x06,
    InvalidHeaderSize                = 0x07,
    InvalidDataSize                  = 0x08,
    InvalidImageType                 = 0x09,
    InvalidTxLength                  = 0x0A,
    InvalidRxLength                  = 0x0B,
    GeneralTxRxError                 = 0x0C,
    ReadDataError                    = 0x0D,
    UnsupportedNumProgramHeaders     = 0x0E,
    InvalidProgramHeaderSize         = 0x0F,
    MultipleSharedSegments           = 0x10,
    UnitializedProgramHeaderLocation = 0x11,
    InvalidDestAddr                  = 0x12,
    InvalidImageHeaderDataSize       = 0x13,
    InvalidELFHeader                 = 0x14,
    UnknownHostError                 = 0x15,
    TimeoutRx                        = 0x16,
    TimeoutTx                        = 0x17,
    InvalidHostMode                  = 0x18,
    InvalidMemoryRead                = 0x19,
    InvalidDataSizeRequest           = 0x1A,
    MemoryDebugNotSupported          = 0x1B,
    InvalidModeSwitch                = 0x1C,
    CommandExecFailure               = 0x1D,
    CommandInvalidParam              = 0x1E,
    CommandUnsupported               = 0x1F,
    InvalidClientCommand             = 0x20,
    HashTableAuthFailure             = 0x21,
    HashTableVerificationFailure     = 0x22,
    HashTableNotFound                = 0x23,
    Limit,
};

enum class Mode : uint32_t {
    ImageTxPending  = 0x00,
    ImageTxComplete = 0x01,
    MemoryDebug     = 0x02,
    Command         = 0x03,
    Limit,
};

enum class ExecCommand : uint32_t {
    NOP                    = 0x00,
    ReadSerialNumber       = 0x01,
    ReadMSMHardwareID      = 0x02,
    ReadOEMPubKeyHashTable = 0x03,
    SwitchDMSS             = 0x04,
    SwitchStreaming        = 0x05,
    ReadDebugData          = 0x06,
    Limit,
};

namespace packet {
struct Header {
    Command  command;
    uint32_t length;
} __attribute__((packed));

struct Hello {
    Header   header = {Command::Hello, sizeof(Hello)};
    uint32_t version;
    uint32_t supported_version;
    uint32_t max_packet_size;
    Mode     mode;
    uint32_t reserved[6];
} __attribute__((packed));

struct HelloResponse {
    Header   header = {Command::HelloResponse, sizeof(HelloResponse)};
    uint32_t version;
    uint32_t supported_version;
    Status   status;
    Mode     mode;
    uint32_t reserved[6];
} __attribute__((packed));

struct ReadData {
    Header   header = {Command::ReadData, sizeof(ReadData)};
    uint32_t image_id;
    uint32_t offset;
    uint32_t size;
} __attribute__((packed));

struct ReadData64 {
    Header   header = {Command::ReadData64, sizeof(ReadData64)};
    uint64_t image_id;
    uint64_t offset;
    uint64_t size;
} __attribute__((packed));

struct EndTransfer {
    Header   header = {Command::EndTransfer, sizeof(EndTransfer)};
    uint32_t image_id;
    Status   status;
} __attribute__((packed));

struct Done {
    Header header = {Command::Done, sizeof(Done)};
} __attribute__((packed));

struct DoneResponse {
    Header header = {Command::DoneResponse, sizeof(DoneResponse)};
    Mode   image_tx_status; // Mode::ImageTxPending or Mode::ImageTxComplete
} __attribute__((packed));

struct Reset {
    Header header = {Command::Reset, sizeof(Reset)};
} __attribute__((packed));

struct ResetResponse {
    Header header = {Command::ResetResponse, sizeof(ResetResponse)};
} __attribute__((packed));

struct SwitchMode {
    Header header = {Command::SwitchMode, sizeof(SwitchMode)};
    Mode   mode;
} __attribute__((packed));

struct Exec {
    Header      header = {Command::Exec, sizeof(Exec)};
    ExecCommand command;
} __attribute__((packed));

struct ExecResponse {
    Header      header = {Command::ExecResponse, sizeof(ExecResponse)};
    ExecCommand client_command;
    uint32_t    data_size;
} __attribute__((packed));

struct ExecData {
    Header      header = {Command::ExecData, sizeof(ExecData)};
    ExecCommand command;
} __attribute__((packed));
} // namespace packet
} // namespace sahara
