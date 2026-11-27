#include "sim.hpp"
#include <exception>
#include <iostream>
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Provide a file" << std::endl;
        return 1;
    }

    try {
        sim_state sim = sim_state::from_solution_file(argv[1]);
        const int timeout = 100000;
        try {
            while (true) {
                sim.step();
                if (sim.get_output_count() >= 120) {
                    break;
                }
                if (sim.get_cycles() >= timeout) {
                    break;
                }
            }
            if (sim.get_output_count() == 120) {
                std::cout << "120th output at cycle " << sim.get_cycles() << std::endl;
                std::cout << "Disclaimer: No full collision detection, no output check" << std::endl;
                std::cout << sim.get_cycles() << "/" << (int)sim.metric_parts << "/" << (int)sim.metric_instrs << std::endl;
            } else {
                int outputs = sim.get_output_count();
                std::cout << "Timeout (cycle " << timeout << " reached with "
                    << outputs << (outputs == 1 ? " output)" : " outputs)") << std::endl;
                std::cout << "?/" << (int)sim.metric_parts << "/" << (int)sim.metric_instrs << std::endl;
            }
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            std::cout << "?/" << (int)sim.metric_parts << "/" << (int)sim.metric_instrs << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error when parsing solution file: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
