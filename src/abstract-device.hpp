#pragma once

class Device {
  public:
    virtual auto clear_rx_buffer() -> bool                = 0;
    virtual auto write(const void* ptr, int size) -> bool = 0;
    virtual auto read(void* ptr, int size) -> int         = 0;
    virtual auto read_struct(void* ptr, int size) -> bool = 0;

    virtual ~Device() {}
};
