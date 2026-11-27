#include "sim.hpp"
#include "geometry.hpp"
#include <exception>
#include <iostream>

static void print_board(std::ostream& os, const std::array<atom, 200>& board) {
    os << std::endl;
    constexpr int row_spaces[13] = {6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6};
    constexpr int row_starts[13] = {6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0};
    constexpr int row_ends[13] = {13, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 7};
    for (int row = 12; row >= 0; row--) {
        for (int i = 0; i < row_spaces[row]; i++) {
            os << ' ';
        }
        for (int col = row_starts[row]; col < row_ends[row]; col++) {
            uint8_t pos = coord_to_offset(col, row);
            atom atom = board[pos];
            uint8_t element = atom.element_and_Hs >> 4;
            if (element > 0) {
                os << "_CNPOSFD"[atom.element_and_Hs >> 4] << "0123456789"[atom.get_Hs()];
            } else {
                os << "__";
            }
        }
        os << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Provide a file" << std::endl;
        return 1;
    }
    try {
        sim_state sim = sim_state::from_solution_file(argv[1]);
        std::cout << (int)sim.metric_parts << "/" << (int)sim.metric_instrs << std::endl;
        try {
            while (sim.get_output_count() < 120) {
                sim.step();
                std::cout << "Cycle " << sim.get_cycles() << ", Output " << (int)sim.get_output_count() << "/120" << std::endl;
                print_board(std::cout, sim.get_board_buf());
                std::cin.get();
            }
        } catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error when parsing solution file: " << e.what();
        return 1;
    }
    return 0;
}
