#ifndef INSTR_HPP
#define INSTR_HPP

#include <array>
#include <cstdint>
enum instr {
    none = 0,
    slide_l,
    slide_r,
    push,
    pull,
    rotate_l,
    rotate_r,
    add_h,
    remove_h,
    trash,
    output,
    shunt_h
};

constexpr std::array<uint8_t, 12> instr_phase = {
    4, 3, 3, 1, 1, 1, 1, 0, 0, 2, 2, 0
};

struct emitter {
    uint8_t direction;
    uint8_t position;
};

#endif
