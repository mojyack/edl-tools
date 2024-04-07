#pragma once
#include "abstract-device.hpp"

auto setup_serial_device(const char* const tty_dev) -> Device*;
