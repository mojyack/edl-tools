#include <optional>
#include <iostream>
#include <string>

#include "firehose-actions.hpp"
#include "sahara-actions.hpp"
#include "serial-device.hpp"
#include "macros/assert.hpp"

auto read_stdin(const std::optional<std::string_view> prompt = std::nullopt) -> std::string {
    if(prompt) {
        std::cout << *prompt;
    }
    auto line = std::string();
    std::getline(std::cin, line);
    return line;
}

auto main(const int argc, const char* const argv[]) -> int {
    ensure(argc == 2, "argc != 2");
    auto dev = setup_serial_device(argv[1]);
    ensure(dev != nullptr);

loop:
    const auto input = read_stdin("EDL% ");
    if(std::cin.eof()) {
        return 0;
    }
    if(input == "hello") {
        ensure(do_command_hello(*dev));
    } else if(input == "switch") {
        dev->clear_rx_buffer();
        ensure(do_switchmode(*dev));
    } else if(input == "reset") {
        dev->clear_rx_buffer();
        ensure(do_reset(*dev));
    } else if(input == "serial") {
        dev->clear_rx_buffer();
        ensure(do_get_serial_number(*dev));
    } else if(input == "hwid") {
        dev->clear_rx_buffer();
        ensure(do_get_msm_hwid(*dev));
    } else if(input == "pkhash") {
        dev->clear_rx_buffer();
        ensure(do_get_pkhash(*dev));
    } else if(input == "upload") {
        ensure(do_upload_hello(*dev, "loader.bin"));
    } else if(input == "fhnop") {
        dev->clear_rx_buffer();
        ensure(fh::send_nop(*dev));
    } else if(input == "fhconf") {
        dev->clear_rx_buffer();
        ensure(fh::send_configure(*dev));
    } else if(input == "fhreset") {
        dev->clear_rx_buffer();
        ensure(fh::send_reset(*dev));
    } else if(input.starts_with("fhread ")) {
        dev->clear_rx_buffer();
        ensure(fh::read_to_file(*dev, input.substr(7)));
    } else if(input.starts_with("fhwrite ")) {
        dev->clear_rx_buffer();
        ensure(fh::write_from_file(*dev, input.substr(8)));
    } else if(input.starts_with("raw ") && input.size() > 4) {
        dev->clear_rx_buffer();
        ensure(dev->write(input.data() + 4, input.size() - 4));
    } else if(input == "refresh") {
        ensure(dev->clear_rx_buffer());
    } else if(input == "help") {
        print("see src/edl-client.cpp");
    } else if(input == "exit") {
        return 0;
    } else {
        print("unknown command");
    }
    goto loop;
}
