#pragma once
#include <string>

auto try_to_dump_packet(const std::byte* buf, const size_t size) -> std::string;
