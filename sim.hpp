#ifndef SIM_HPP
#define SIM_HPP

#include "atom.hpp"
#include "parts.hpp"
#include <array>
#include <cstdint>

/*
 * The board is stored virtually in a 16 * 16 buffer. For a hex board with side length 7, it looks like this:
 *
 * ######.......###
 * #####........###
 * ####.........###
 * ###..........###
 * ##...........###
 * #............###
 * .............###
 * ............####
 * ...........#####
 * ..........######
 * .........#######
 * ........########
 * .......#########
 * ################
 * ################
 * ################
 *
 * I am actually only storing the cells up to the last cell, which will be 12 * 16 + 7 = 199. Padding to 200.
 * Positions will be stored as an offset in the grid in a uint8_t, up to 256.
 * Positions larger than 200 (including underflows) will be wrapped to 0, where it is a wall.
 *
 * Save file coordinates have to be converted to my format by adding 6 to every coordinate.
 */

class sim_state {
    const std::array<uint8_t, 6 * 24> instrs;
    const uint8_t instr_len;
    const uint8_t puzzle_id;
    const uint8_t precursor_count;
    // This is the upper bound, but in most cases there are much fewer. Can we save memory here?
    const std::array<precursor_part, 127> precursors;
    std::array<emitter_part, 6> emitters;
    std::array<atom, 200> atom_buf;
    uint8_t pc;
    uint8_t outputs;
    uint64_t cycles;

    constexpr static std::array<atom, 200> empty_board = []() {
        std::array<atom, 200> res{EMPTY_CELL};
        constexpr int row_starts[12] = {6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0};
        constexpr int row_ends[12] = {13, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8};
        for (int row = 0; row < 12; row++) {
            for (int col = 0; col < row_starts[row]; col++) {
                res[row * 16 + col] = WALL;
            }
            for (int col = row_ends[row]; col < 16; col++) {
                res[row * 16 + col] = WALL;
            }
        }
        res[199] = WALL;
        return res;
    }();

    // Gets the atom, also in case pos is out of bounds, where it will return a WALL.
    // Directly accessing atom_buf can read out of the memory. Only do that when pos is known to be in bounds.
    inline atom get_atom(uint8_t pos);
    inline uint8_t get_curr_instr(uint8_t emitter_id);
    void flood_fill_movement_molecule(uint8_t pos, std::array<uint16_t, 200> &emitter_map, uint16_t value, uint8_t &atom_i, std::array<uint8_t, 127> &atom_buf);
    void flood_fill_erase_molecule(uint8_t pos);

sim_state(const std::array<uint8_t, 6 * 24> instrs,
    const uint8_t instr_len,
    const uint8_t puzzle_id,
    const uint8_t precursor_count,
    const uint8_t metric_parts,
    const uint8_t metric_instrs,
    const std::array<precursor_part, 127> precursors,
    std::array<emitter_part, 6> emitters
) : instrs{instrs}, instr_len{instr_len}, puzzle_id{puzzle_id}, precursor_count{precursor_count},
    metric_parts{metric_parts}, metric_instrs{metric_instrs}, precursors{precursors}, emitters{emitters},
   atom_buf{empty_board}, pc{0}, outputs{0}, cycles{0} {}

public:
    const uint8_t metric_parts;
    const uint8_t metric_instrs;
    static sim_state from_solution_file(const char* path);
    const std::array<atom, 200>& get_board_buf() const {
        return atom_buf;
    }
    void step();
    uint8_t get_output_count() const {
        return outputs;
    }
    uint64_t get_cycles() const {
        return cycles;
    }
};

#endif
