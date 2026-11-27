#include "sim.hpp"
#include "atom.hpp"
#include "byte_reader.hpp"
#include "geometry.hpp"
#include "instr.hpp"
#include "parts.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

inline atom sim_state::get_atom(uint8_t pos) {
    uint8_t wrapped = pos >= 200 ? 0 : pos;
    return atom_buf[wrapped];
}

inline uint8_t sim_state::get_curr_instr(uint8_t emitter_id) {
    return instrs[pc * 6 + emitter_id];
}

struct movement {
    uint8_t molecule_begin;
    uint8_t molecule_end;
    uint8_t dir;
    uint8_t instr;
    uint8_t pivot;
};

void sim_state::flood_fill_movement_molecule(uint8_t pos, std::array<uint16_t, 200>& emitter_map, uint16_t value, uint8_t& atom_i, std::array<uint8_t, 127>& atom_poss) {
    emitter_map[pos] = value;
    atom_poss[atom_i++] = pos;
    atom atom = atom_buf[pos];
    auto bonds = atom.bonds;
    for (int dir = 0; dir < 6; dir++) {
        auto offset = dir_to_offset[dir];
        if ((bonds & 3) > 0 && emitter_map[(uint8_t)(pos + offset)] == 0) {
            flood_fill_movement_molecule((uint8_t)(pos + offset), emitter_map, value, atom_i, atom_poss);
        }
        bonds >>= 2;
    }
}

void sim_state::flood_fill_erase_molecule(uint8_t pos) {
    auto bonds = atom_buf[pos].bonds;
    atom_buf[pos] = EMPTY_CELL;
    for (int dir = 0; dir < 6; dir++) {
        auto offset = dir_to_offset[dir];
        if ((bonds & 3) > 0 && atom_buf[(uint8_t)(pos + offset)] != EMPTY_CELL) {
            flood_fill_erase_molecule((uint8_t)(pos + offset));
        }
        bonds >>= 2;
    }
}

void sim_state::step() {
    // Add precursors to the board if all cells are free
    for (int i = 0; i < precursor_count; i++) {
        auto& p = precursors[i];
        bool all_unoccupied = true;
        for (int j = precursors_bank.offsets[p.id]; j < precursors_bank.offsets[p.id + 1]; j++) {
            int8_t q = precursors_bank.positions[j * 2], r = precursors_bank.positions[j * 2 + 1];
            uint8_t atom_pos = rotate_pos(q, r, p.rotation);
            if (atom_buf[(uint8_t)(p.position + atom_pos)] != EMPTY_CELL) {
                all_unoccupied = false;
                break;
            }
        }
        if (all_unoccupied) {
            for (int j = precursors_bank.offsets[p.id]; j < precursors_bank.offsets[p.id + 1]; j++) {
                int8_t q = precursors_bank.positions[j * 2], r = precursors_bank.positions[j * 2 + 1];
                uint8_t atom_pos = rotate_pos(q, r, p.rotation);
                atom atom = precursors_bank.atoms[j];
                atom.rotate(p.rotation);
                atom_buf[(uint8_t)(p.position + atom_pos)] = atom;
            }
        }
    }

    std::array<uint16_t, 200> emitter_dir_map_debond{0};
    std::array<uint16_t, 200> emitter_dir_map_bond{0};
    std::array<movement, 6> movements;
    uint8_t movement_count = 0;
    std::array<uint8_t, 127> movement_atom_pos_buf;
    std::array<uint8_t, 5> emitters_by_phase_count{0};
    std::array<uint8_t, 6> emitters_by_phase[5];

    for (auto &emitter: emitters) {
        auto instr = get_curr_instr(emitter.id);
        auto phase = instr_phase[instr];
        emitters_by_phase[phase][emitters_by_phase_count[phase]++] = emitter.id;
    }

    // I'm collecting info from movement related instrs first.
    uint8_t movement_atom_i = 0;
    for (int i = 0; i < emitters_by_phase_count[1]; i++) {
        auto emitter_i = emitters_by_phase[1][i];
        auto emitter = emitters[emitter_i];
        uint8_t emitter_grid_pos = hex_pos_to_emitter_pos[emitter.pos];
        uint8_t offset = dir_to_offset[emitter.dir];
        uint8_t hit_pos = emitter_grid_pos + offset;
        for (; get_atom(hit_pos) == EMPTY_CELL; hit_pos += offset) {}

        if (get_atom(hit_pos) == WALL) {
            continue;
        }

        if (emitter_dir_map_bond[hit_pos] == 0) {
            movements[movement_count].molecule_begin = movement_atom_i;
            constexpr uint8_t opposite_dir[6] = {3, 4, 5, 0, 1, 2};
            uint8_t opposite = opposite_dir[emitter.dir];
            flood_fill_movement_molecule(hit_pos, emitter_dir_map_bond, 0b1000001 << opposite, movement_atom_i, movement_atom_pos_buf);
            movements[movement_count].molecule_end = movement_atom_i;
            movements[movement_count].dir = emitter.dir;
            movements[movement_count].instr = get_curr_instr(emitter.id);
            movements[movement_count].pivot = hit_pos;
            movement_count++;
        }
    }

    for (int i = 0; i < emitters_by_phase_count[0]; i++) {
        auto emitter_i = emitters_by_phase[0][i];
        auto emitter = emitters[emitter_i];
        uint8_t emitter_grid_pos = hex_pos_to_emitter_pos[emitter.pos];
        uint8_t offset = dir_to_offset[emitter.dir];
        uint8_t hit_pos = emitter_grid_pos + offset;
        for (; get_atom(hit_pos) == EMPTY_CELL; hit_pos += offset) {}

        if (get_atom(hit_pos) == WALL) {
            continue;
        }

        const auto sim_add_H = [&](uint8_t pos) {
            constexpr uint8_t opposite_and_mirrored_dir[6] = {2, 1, 0, 5, 4, 3};
            uint8_t opposite = opposite_and_mirrored_dir[emitter.dir];
            emitter_dir_map_debond[pos] |= 0b1000001 << opposite;
            if (atom_buf[pos].get_Hs() < max_bonds[atom_buf[pos].get_element()]) {
                atom_buf[pos].add_H_unsafe();
            }
        };

        const auto sim_remove_H = [&](uint8_t pos) {
            if (atom_buf[pos].get_Hs() == 0) {
                return;
            }
            atom_buf[pos].remove_H_unsafe();
            constexpr uint8_t opposite_dir[6] = {3, 4, 5, 0, 1, 2};
            uint8_t opposite = opposite_dir[emitter.dir];
            emitter_dir_map_bond[pos] |= 0b1000001 << opposite;
        };

        switch (get_curr_instr(emitter.id)) {
            case add_h: {
                sim_add_H(hit_pos);
                break;
            }
            case remove_h: {
                sim_remove_H(hit_pos);
                break;
            }
            case shunt_h: {
                sim_remove_H(hit_pos);
                uint8_t hit_pos2 = hit_pos + offset;
                for (; get_atom(hit_pos2) == EMPTY_CELL; hit_pos2 += offset) {}
                if (get_atom(hit_pos2) == WALL) {
                    break;
                }
                sim_add_H(hit_pos2);
                break;
            }
            default:
                std::unreachable();
        }
    }

    // TODO: detect collisions
    // To anyone who wants to help writing this:
    //
    // All molecule movements are collected in the movements array with movement_count as length.
    // Iterate from movement_atom_pos_buf[movement.molecule_begin] to movement_atom_pos_buf[movement.molecule_end]
    // to iterate over all atom positions in a molecule.
    // See the move and rotate code below for example usage.
    //
    // Atoms that are not moved are not collected and can be only accessed by atom_buf.
    //
    // Use get_screen_position(pos) from geometry.hpp to get the position of a cell on the screen.
    // Use get_atom_diameter() from geometry.hpp to get the diameter of an atom.
    //
    // The game samples 100 times every cycle.
    //
    // Direction of 0 points to the right. Increases in the ccw direction.
    {

    }

    struct _bond {
        uint8_t pos;
        uint8_t bond_dir; // one of 0, 1, 2
        uint16_t get_positional_priority() { // Optimally 10 bits, here 16 bits for convenience
            uint8_t opposite = ~pos;
            int r = opposite >> 4;
            int q = opposite & 15;
            int is_flat = bond_dir == 0;
            return (uint16_t)(r << 12 | is_flat << 8 | q << 4 | bond_dir);
        }
        _bond opposite() {
            uint8_t offset = dir_to_offset[bond_dir];
            constexpr uint8_t opposite_dir[6] = {3, 4, 5, 0, 1, 2};
            return {(uint8_t)(pos + offset), opposite_dir[bond_dir]};
        }
    };
    // debond
    {
        while (true) {
            // TODO: Scanning the whole board is unnecessary. Do some trimming
            uint32_t best_prio = 0;
            _bond best_bond;
            for (uint8_t pos = 6; pos < 199; pos++) {
                if (atom_buf[pos].energy >= 0) { continue; }
                uint16_t bonds_all = atom_buf[pos].bonds;
                for (uint8_t dir = 0; dir < 6; dir++, bonds_all >>= 2) {
                    int bonds = bonds_all & 3;   // prio 1, 2 bits
                    if (bonds == 0) { continue; }
                    uint8_t offset = dir_to_offset[dir];
                    uint8_t other_pos = pos + offset;
                    uint8_t e_l, e_h;            // prio 2.1 & 2.2, 6 bits total
                    std::tie(e_l, e_h) = std::minmax(atom_buf[pos].get_element(), atom_buf[other_pos].get_element());
                    uint8_t prio_1_2 = bonds << 6 | e_l << 3 | e_h;
                    uint16_t emitters1 = emitter_dir_map_debond[pos] >> (5 - dir);
                    constexpr uint8_t opposite_and_mirrored_dir[6] = {2, 1, 0, 5, 4, 3};
                    uint16_t emitters2 = emitter_dir_map_debond[other_pos] >> opposite_and_mirrored_dir[dir];
                    uint16_t emitters = emitters1 | emitters2;
                    uint8_t prio3 = ~(emitters ^ (emitters - 1)); // prio 3, optimally 6 bits
                    _bond b;
                    if (dir < 3) {
                        b = {pos, dir};
                    } else {
                        b = {other_pos, (uint8_t)(dir - 3)};
                    }
                    uint16_t prio4 = b.get_positional_priority();
                    uint32_t prio = prio_1_2 << 24 | prio3 << 16 | prio4;
                    if (prio > best_prio) {
                        best_prio = prio;
                        best_bond = b;
                    }
                }
            }
            if (best_prio == 0) break;
            atom_buf[best_bond.pos].debond_unsafe(best_bond.bond_dir);
            _bond opposite = best_bond.opposite();
            atom_buf[opposite.pos].debond_unsafe(opposite.bond_dir);
        }
    }

    // move or rotate
    {
        std::array<atom, 200> atom_buf_prev = atom_buf;
        std::array<uint16_t, 200> emitter_dir_map_bond_prev = emitter_dir_map_bond;
        for (int i = 0; i < movement_atom_i; i++) {
            atom_buf[movement_atom_pos_buf[i]] = EMPTY_CELL;
            emitter_dir_map_bond[movement_atom_pos_buf[i]] = 0;
        }
        for (int i = 0; i < movement_count; i++) {
            movement mv = movements[i];
            const uint8_t offset = dir_to_offset[mv.dir];
            for (uint8_t j = mv.molecule_begin; j < mv.molecule_end; j++) {
                uint8_t pos_prev = movement_atom_pos_buf[j];
                atom prev = atom_buf_prev[pos_prev];
                uint8_t pos_after;
                switch(mv.instr) {
                    case push: pos_after = pos_prev + offset; break;
                    case pull: pos_after = pos_prev - offset; break;
                    case rotate_l:
                    case rotate_r:
                        prev.rotate_by_60(mv.instr == rotate_l);
                        pos_after = rotate_pos_by_60(pos_prev, mv.pivot, mv.instr == rotate_l);
                        break;
                    default:
                        std::unreachable();
                }
                if (get_atom(pos_after) != EMPTY_CELL) {
                    throw std::runtime_error("Collision at end of movement (not a full collision detection)");
                }
                atom_buf[pos_after] = prev;
                emitter_dir_map_bond[pos_after] = emitter_dir_map_bond_prev[pos_prev];
            }
        }
    }

    // Bond
    {
        while (true) {
            // TODO: Scanning the whole board is unnecessary. Do some trimming
            uint32_t best_prio = 0xFFFFFFFF;
            _bond best_bond;
            for (uint8_t pos = 6; pos < 199; pos++) {
                if (atom_buf[pos].energy <= 0) { continue; }
                uint16_t bonds_all = atom_buf[pos].bonds;
                // Due to the symmetry I will only iterate half of all directions
                for (uint8_t dir = 0; dir < 3; dir++, bonds_all >>= 2) {
                    int bonds = bonds_all & 3;   // prio 1, 2 bits
                    uint8_t offset = dir_to_offset[dir];
                    uint8_t other_pos = pos + offset;
                    if (bonds == 3 || atom_buf[other_pos].energy <= 0) { continue; }
                    uint8_t e_l, e_h;            // prio 2.1 & 2.2, 6 bits total
                    std::tie(e_l, e_h) = std::minmax(atom_buf[pos].get_element(), atom_buf[other_pos].get_element());
                    uint8_t prio_1_2 = bonds << 6 | e_l << 3 | e_h;
                    uint16_t emitters1 = emitter_dir_map_bond[pos] >> (dir + 1);
                    constexpr uint8_t opposite_dir[6] = {3, 4, 5, 0, 1, 2};
                    uint16_t emitters2 = emitter_dir_map_bond[other_pos] >> (opposite_dir[dir] + 1);
                    uint16_t emitters = emitters1 | emitters2;
                    uint8_t prio3 = emitters ^ (emitters - 1); // prio 3, optimally 6 bits
                    _bond b = {pos, dir};
                    uint16_t prio4 = b.get_positional_priority();
                    uint32_t prio = prio_1_2 << 24 | prio3 << 16 | prio4;
                    if (prio < best_prio) {
                        best_prio = prio;
                        best_bond = b;
                    }
                }
            }
            if (best_prio == 0xFFFFFFFF) break;
            atom_buf[best_bond.pos].add_bond_unsafe(best_bond.bond_dir);
            _bond opposite = best_bond.opposite();
            atom_buf[opposite.pos].add_bond_unsafe(opposite.bond_dir);
        }
    }

    // erase or output
    for (int i = 0; i < emitters_by_phase_count[2]; i++) {
        auto emitter_i = emitters_by_phase[2][i];
        auto emitter = emitters[emitter_i];
        uint8_t emitter_grid_pos = hex_pos_to_emitter_pos[emitter.pos];
        uint8_t offset = dir_to_offset[emitter.dir];
        uint8_t hit_pos = emitter_grid_pos + offset;
        for (; get_atom(hit_pos) == EMPTY_CELL; hit_pos += offset) {}

        if (get_atom(hit_pos) == WALL) {
            continue;
        }
        flood_fill_erase_molecule(hit_pos);
        // TODO: check output (should be done before erasing)
        // Doable in O(n), though the easy idea would require 6 scans in worse case.
        // Preprocessing the output molecule in a suffix-array-ish way can reduce that into one scan,
        // but this preprocess is not simple (for me), so it's a decision that if it's worth it to
        // do the optimization, as this check will only happen 120 times each sim.
        //
        // This will affect how I am storing the puzzle data, and there isn't a small number of puzzles.
        // Should be decided before setting up puzzles.
        // Main problem is that changing it after all puzzles are implemented is super tedious.
        if (get_curr_instr(emitter_i) == output) {
            outputs++;
        }
    }

    // slide emitters
    for (int i = 0; i < emitters_by_phase_count[3]; i++) {
        auto emitter_i = emitters_by_phase[3][i];
        auto &emitter = emitters[emitter_i];
        if (instrs[pc * 6 + emitter_i] == instr::slide_r) {
            if (emitter.pos < 41) {
                emitter.pos++;
            } else {
                emitter.pos = 0;
            }
        } else {
            if (emitter.pos > 0) {
                emitter.pos--;
            } else {
                emitter.pos = 41;
            }
        }
    }

    pc++;
    if (pc >= instr_len) pc = 0;
    cycles++;
}

sim_state sim_state::from_solution_file(const char* path) {
    byte_reader reader{path};

    std::array<uint8_t, 6 * 24> _instrs;
    uint8_t _instr_len = 0;
    uint8_t _precursor_count = 0;
    std::array<precursor_part, 127> _precursors;
    std::array<emitter_part, 6> _emitters;
    uint8_t _emitter_part_count = 0;
    uint8_t _metric_instrs = 0;

    int32_t version = reader.read_int32_le();
    if (version < 1003) {
        throw std::runtime_error("Unsupported version");
    }
    int32_t _puzzle_id = reader.read_int32_le();
    reader.skip_string();
    int32_t metric_count = reader.read_int32_le();
    int32_t _metrics[3] = {-1, -1, -1};
    for (int i = 0; i < metric_count; i++) {
        int32_t metric_type = reader.read_int32_le();
        int32_t val = reader.read_int32_le();
        if (metric_type >= 0 && metric_type < 3) {
            _metrics[metric_type] = val;
        }
    }
    int32_t part_count = reader.read_int32_le();
    int emitters_appeared = 0;
    for (int i = 0; i < part_count; i++) {
        int32_t part_type = reader.read_int32_le();
        int32_t q = reader.read_int32_le();
        int32_t r = reader.read_int32_le();
        uint8_t pos = coord_to_offset(q + 6, r + 6);
        uint8_t dir = (reader.read_int32_le() % 6 + 6) % 6;
        if (reader.read_bool()) {
            int32_t precursor_id = reader.read_int32_le();
            reader.read_int32_le();
            reader.skip_string();
            precursor_id--;
            if (precursor_id < 0 || precursor_id >= 23) {
                throw std::runtime_error("Invalid Precursor ID");
            }
            _precursors[_precursor_count++] = {(uint8_t)precursor_id, dir, pos};
            // TODO: Check availability in puzzle, check overlapping, bonds check
        } else {
            int32_t emitter_id = reader.read_int32_le();
            if (emitter_id < 0 || emitter_id >= 6) {
                throw std::runtime_error("Emitter ID must be an integer in [0, 5]");
            }
            if (emitters_appeared & (1 << emitter_id)) {
                throw std::runtime_error("Emitter ID duplicated. This solution cannot be created in the game.");
            }
            emitters_appeared |= (1 << emitter_id);

            uint8_t emitter_pos;
            for (emitter_pos = 0; emitter_pos < 42; emitter_pos++) {
                if (hex_pos_to_emitter_pos[emitter_pos] == pos) {
                    break;
                }
            }
            if (emitter_pos == 42) {
                throw std::runtime_error("Emitter not on the outer ring of the board. This solution cannot be created in the game.");
            }

            int32_t instrs = reader.read_int32_le();
            if (instrs > 24) {
                throw std::runtime_error("Too many instructions on an emitter. This solution cannot be created in the game.");
            }
            uint8_t last_instr = 0;
            for (int j = 0; j < instrs; j++) {
                uint8_t instr = reader.read_byte();
                if (instr > 11) {
                    throw std::runtime_error("Invalid instruction");
                }
                _instrs[6 * j + emitter_id] = instr;
                if (instr != 0) {
                    last_instr = j + 1;
                    _metric_instrs++;
                }
            }
            _emitters[emitter_id] = {(uint8_t)emitter_id, dir, emitter_pos};
            if (last_instr > 0) {
                _emitter_part_count++;
            }
            _instr_len = std::max(_instr_len, last_instr);
        }
    }
    return sim_state{_instrs, _instr_len, (uint8_t)_puzzle_id, _precursor_count,
        (uint8_t)(_precursor_count + _emitter_part_count), _metric_instrs, _precursors, _emitters};
}
