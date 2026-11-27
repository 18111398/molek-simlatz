#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <array>
#include <cstdint>
#include <utility>

constexpr inline uint8_t coord_to_offset(int8_t q, int8_t r) {
    return (uint8_t)(r * 16 + q);
}

constexpr uint8_t dir_to_offset[6] = {
    coord_to_offset(1, 0),
    coord_to_offset(0, 1),
    coord_to_offset(-1, 1),
    coord_to_offset(-1, 0),
    coord_to_offset(0, -1),
    coord_to_offset(1, -1)
};

constexpr std::array<uint8_t, 42> hex_pos_to_emitter_pos = []()  {
    std::array<uint8_t, 42> res{};
    constexpr uint8_t corners[6] = {
        coord_to_offset(13, 6),
        coord_to_offset(6, 13),
        coord_to_offset(-1, 13),
        coord_to_offset(-1, 6),
        coord_to_offset(6, -1),
        coord_to_offset(13, -1)
    };
    // I'm not sure where to start, so the code is made to make the starting point easily adjustable.
    int starting_point = 0;
    for (int i = 0; i < 6; i++) {
        uint8_t corner = corners[(starting_point + i) % 6];
        uint8_t offset = dir_to_offset[(starting_point + i + 2) % 6];
        for (int j = 0; j < 7; j++) {
            res[i * 7 + j] = corner + offset * j;
        }
    }
    return res;
}();

// Sufficient for rotations in sim
constexpr uint8_t rotate_pos_by_60(uint8_t pos, uint8_t pivot, bool is_ccw) {
    int8_t dq = pos % 16 - pivot % 16;
    int8_t dr = pos / 16 - pivot / 16;
    int8_t d;
    if (is_ccw) {
        d = coord_to_offset(-dr, dq + dr);
    } else {
        d = coord_to_offset(dq + dr, -dq);
    }
    return d + pivot;
}

// Used for precursors and output matching
constexpr uint8_t rotate_pos(int8_t dq, int8_t dr, uint8_t dir) {
    int8_t d;
    switch (dir) {
        case 0: // Doesn't do anything. Save the work?
            d = coord_to_offset(dq, dr); break;
        case 1:
            d = coord_to_offset(-dr, dq + dr); break;
        case 2:
            d = coord_to_offset(-dq - dr, dq); break;
        case 3:
            d = coord_to_offset(-dq, -dr); break;
        case 4:
            d = coord_to_offset(dr, -dq - dr); break;
        case 5:
            d = coord_to_offset(dq + dr, -dq); break;
        default:
            std::unreachable();
    }
    return d;
}

constexpr std::pair<float, float> get_screen_position(uint8_t pos) {
    int q = pos % 16;
    int r = pos / 16;
    return {q * 34.f + r * 17.f, r * 30.f};
}

constexpr float get_atom_diameter() {
    return 29.f;
}
#endif
