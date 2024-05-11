#pragma once

namespace buse {
struct Operator;

auto run(const char* nbd_path, Operator& op) -> int;
} // namespace buse
