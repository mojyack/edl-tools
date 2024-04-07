#include <optional>

#include "assert.hpp"
#include "firehose-actions.hpp"
#include "sahara-actions.hpp"
#include "serial-device.hpp"

auto read_stdin(const std::optional<std::string_view> prompt = std::nullopt) -> std::string {
    if(prompt) {
        std::cout << *prompt;
    }
    auto line = std::string();
    std::getline(std::cin, line);
    return line;
}

auto main(const int argc, const char* const argv[]) -> int {
    assert_v(argc == 2, 1, "argc != 2");
    auto dev = setup_serial_device(argv[1]);
    assert_v(dev != nullptr, 1);

loop:
    const auto input = read_stdin("EDL% ");
    if(std::cin.eof()) {
        return 0;
    }
    if(input == "hello") {
        assert_v(do_command_hello(*dev), -1);
    } else if(input == "switch") {
        dev->clear_rx_buffer();
        assert_v(do_switchmode(*dev), -1);
    } else if(input == "reset") {
        dev->clear_rx_buffer();
        assert_v(do_reset(*dev), -1);
    } else if(input == "serial") {
        dev->clear_rx_buffer();
        assert_v(do_get_serial_number(*dev), -1);
    } else if(input == "hwid") {
        dev->clear_rx_buffer();
        assert_v(do_get_msm_hwid(*dev), -1);
    } else if(input == "pkhash") {
        dev->clear_rx_buffer();
        assert_v(do_get_pkhash(*dev), -1);
    } else if(input == "upload") {
        assert_v(do_upload_hello(*dev, "loader.bin"), -1);
    } else if(input == "fhnop") {
        dev->clear_rx_buffer();
        assert_v(fh::send_nop(*dev), -1);
    } else if(input == "fhconf") {
        dev->clear_rx_buffer();
        assert_v(fh::send_configure(*dev), -1);
    } else if(input == "fhreset") {
        dev->clear_rx_buffer();
        assert_v(fh::send_reset(*dev), -1);
    } else if(input.starts_with("fhread ")) {
        dev->clear_rx_buffer();
        fh::read_to_file(*dev, input.substr(7));
    } else if(input.starts_with("fhwrite ")) {
        dev->clear_rx_buffer();
        fh::write_from_file(*dev, input.substr(8));
    } else if(input.starts_with("raw ") && input.size() > 4) {
        dev->clear_rx_buffer();
        dev->write(input.data() + 4, input.size() - 4);
    } else if(input == "refresh") {
        dev->clear_rx_buffer();
    } else if(input == "help") {
        print("see src/edl-client.cpp");
    } else if(input == "exit") {
        return 0;
    } else {
        print("unknown command");
    }
    goto loop;
}
