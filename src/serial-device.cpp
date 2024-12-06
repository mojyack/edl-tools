#include <cstring>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "abstract-device.hpp"
#include "config.hpp"
#include "macros/assert.hpp"
#include "sahara-packet-stringnize.hpp"
#include "util/fd.hpp"

namespace {
auto dump_bin(const std::byte* const ptr, const size_t size) -> void {
    for(auto i = 0u; i < size; i += 1) {
        printf("%02x", (unsigned int)ptr[i]);
        if((i + 1) % 4 == 0) {
            if((i + 1) % 32 == 0) {
                printf("\n");
            } else {
                printf(" ");
            }
        }
    }
    printf("\n");
}

auto dump_packet(const std::byte* const ptr, const size_t size) -> void {
    if(size == 0) {
        return;
    }

    if(const auto str = std::string_view(std::bit_cast<const char*>(ptr), size); str.starts_with("<?xml")) {
        print(str);
        return;
    }

    if(size >= 0xff) {
        return;
    }

    dump_bin(ptr, size);

    const auto str = try_to_dump_packet(ptr, size);
    if(!str.empty()) {
        print(str);
    }
}
} // namespace

class SerialDevice : public Device {
  private:
    FileDescriptor fd;

  public:
    auto clear_rx_buffer() -> bool override {
        print("clearing received data");
        ensure(tcflush(fd.as_handle(), TCIFLUSH) == 0);
        return true;
    }

    auto write(const void* const ptr, const int size) -> bool override {
        if(config::dump_serial_io) {
            print("<- ", size);
            dump_packet((std::byte*)ptr, size);
        }
        return fd.write(ptr, size);
    }

    auto read(void* const ptr, const int size) -> int override {
        const auto res = ::read(fd.as_handle(), ptr, size);
        if(config::dump_serial_io) {
            print("-> ", res);
            dump_packet((std::byte*)ptr, res);
        }
        return res;
    }

    auto read_struct(void* const ptr, const int size) -> bool override {
        const auto res = fd.read(ptr, size);
        if(config::dump_serial_io) {
            print("-> ", size);
            dump_packet((std::byte*)ptr, size);
        }
        return res;
    }

    SerialDevice(FileDescriptor fd) : fd(std::move(fd)) {}

    static auto setup(const char* const tty_dev) -> SerialDevice* {
        const auto devfd = open(tty_dev, O_RDWR);
        ensure(devfd >= 0, "failed to open device: ", strerror(errno));
        auto dev = FileDescriptor(devfd);

        auto tio = termios{};
        memset(&tio, 0, sizeof(tio));
        tio.c_cflag = CREAD | CLOCAL | CS8;
        ensure(cfsetispeed(&tio, B115200) == 0);
        ensure(cfsetospeed(&tio, B115200) == 0);
        cfmakeraw(&tio);
        ensure(tcsetattr(devfd, TCSANOW, &tio) == 0);
        ensure(ioctl(devfd, TCSETS, &tio) == 0, "setup tty failed: ", strerror(errno));

        return new SerialDevice(std::move(dev));
    }
};

auto setup_serial_device(const char* const tty_dev) -> Device* {
    return SerialDevice::setup(tty_dev);
}
