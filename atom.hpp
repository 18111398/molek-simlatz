#ifndef ATOM_HPP
#define ATOM_HPP

#include <cstdint>
#include <string.h>


enum element {
    C = 1,
    N,
    P,
    O,
    S,
    F,
    Cl
};

constexpr uint8_t max_bonds[8] = {
    0, 4, 3, 5, 2, 6, 1, 1
};

struct atom {
    uint16_t bonds;
    int8_t energy;
    uint8_t element_and_Hs; // higher four bits represent the element, lower four bits the number of Hs.


    constexpr static atom make(element element, uint8_t Hs, uint8_t bonds0, uint8_t bonds1, uint8_t bonds2, uint8_t bonds3, uint8_t bonds4, uint8_t bonds5) {
        atom res;
        res.bonds = (bonds5 << 10) | (bonds4 << 8) | (bonds3 << 6) | (bonds2 << 4) | (bonds1 << 2) | bonds0;
        res.energy = max_bonds[element] - Hs - bonds0 - bonds1 - bonds2 - bonds3 - bonds4 - bonds5;
        res.element_and_Hs = (element << 4) | Hs;
        return res;
    }

    inline void rotate_by_60(bool is_ccw) {
        bonds = is_ccw ?
            (bonds >> 10 | (bonds & 1023) << 2) :
            (bonds >> 2 | (bonds & 3) << 10);
    }

    inline void rotate(uint8_t dir) {
        bonds = ((bonds << (dir * 2)) & ((1 << 12) - 1)) | (bonds >> ((6 - dir) * 2));
    }

    constexpr inline uint8_t get_Hs() const {
        return element_and_Hs & 15;
    }

    constexpr inline uint8_t get_element() const {
        return element_and_Hs >> 4;
    }

    inline void add_bond_unsafe(char rotation) {
        bonds += 1 << (rotation * 2);
        energy -= 1;
    }

    inline void debond_unsafe(char rotation) {
        bonds -= 1 << (rotation * 2);
        energy += 1;
    }

    inline void add_H_unsafe() {
        element_and_Hs += 1;
        energy -= 1;
    }

    inline void remove_H_unsafe() {
        element_and_Hs -= 1;
        energy += 1;
    }

    inline bool operator==(atom other) {
        return memcmp(this, &other, sizeof(atom)) == 0;
    }
};

constexpr atom EMPTY_CELL = {0, 0, 0};
// Walls are represented by an impossible atom case, where the element is none but there are 15 Hs.
constexpr atom WALL = {0, 0, 15};

#endif // ATOM_HPP
