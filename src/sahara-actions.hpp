#include <string_view>

#include "abstract-device.hpp"

auto do_command_hello(Device& dev) -> bool;
auto do_switchmode(Device& dev) -> bool;
auto do_reset(Device& dev) -> bool;
auto do_get_serial_number(Device& dev) -> bool;
auto do_get_msm_hwid(Device& dev) -> bool;
auto do_get_pkhash(Device& dev) -> bool;
auto do_upload_hello(Device& dev, std::string_view programmer) -> bool;

