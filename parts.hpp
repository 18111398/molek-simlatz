#ifndef PARTS_HPP
#define PARTS_HPP
#include "atom.hpp"
#include <array>
#include <cstdint>

struct emitter_part {
    uint8_t id;    // Should be const, but makes initialization trickier
    uint8_t dir;   // Should be const, but makes initialization trickier
    uint8_t pos;
};

struct precursor_part {
    uint8_t id;
    uint8_t rotation;
    uint8_t position;
};

struct precursor_bank {
    std::array<uint8_t, 24> offsets;
    std::array<uint8_t, 88 * 2> positions;
    std::array<atom, 88> atoms;
};

struct temp_precursor {
    uint8_t atom_count;
    std::array<int8_t, 14> positions;
    std::array<atom, 7> atoms;
};

constexpr precursor_bank precursors_bank = [](const int n, const std::array<temp_precursor, 23> temps) consteval {
    struct precursor_bank bank{0};
    bank.offsets[0] = 0;
    for (int i = 0; i < n; i++) {
        temp_precursor temp = temps[i];
        for (int j = 0; j < temp.atom_count; j++) {
            bank.positions[(bank.offsets[i] + j) * 2] = temp.positions[j * 2];
            bank.positions[(bank.offsets[i] + j) * 2 + 1] = temp.positions[j * 2 + 1];
            bank.atoms[bank.offsets[i] + j] = temp.atoms[j];
        }
        bank.offsets[i + 1] = bank.offsets[i] + temp.atom_count;
    }
    return bank;
}(23, std::array<temp_precursor, 23> {
    // H20, Water
    temp_precursor{1, {0, 0}, {atom::make(O, 2, 0, 0, 0, 0, 0, 0)}},
    // NH3, Ammonia
    temp_precursor{1, {0, 0}, {atom::make(N, 3, 0, 0, 0, 0, 0, 0)}},
    // HCl, Hydrochloric Acid
    temp_precursor{1, {0, 0}, {atom::make(Cl, 1, 0, 0, 0, 0, 0, 0)}},
    // H2SO4, Sulfuric Acid
    temp_precursor{5, {
        0, 0, 1, 0, 0, 1, -1, 0, 0, -1
    }, {
        atom::make(S, 0, 1, 2, 0, 1, 2, 0),
        atom::make(O, 1, 0, 0, 0, 1, 0, 0),
        atom::make(O, 0, 0, 0, 0, 0, 2, 0),
        atom::make(O, 1, 1, 0, 0, 0, 0, 0),
        atom::make(O, 0, 0, 2, 0, 0, 0, 0),
    }},
    // CH3COCH3, Acetone
    temp_precursor{4, {
        0, 0, 0, 1, -1, 0, 1, -1
    }, {
        atom::make(C, 0, 0, 2, 0, 1, 0, 1),
        atom::make(O, 0, 0, 0, 0, 0, 2, 0),
        atom::make(C, 3, 1, 0, 0, 0, 0, 0),
        atom::make(C, 3, 0, 0, 1, 0, 0, 0),
    }},
    // C6H12, Cyclohexane
    temp_precursor{6, {
        0, 0, 1, 0, 2, -1, 2, -2, 1, -2, 0, -1
    }, {
        atom::make(C, 2, 1, 0, 0, 0, 1, 0),
        atom::make(C, 2, 0, 0, 0, 1, 0, 1),
        atom::make(C, 2, 0, 0, 1, 0, 1, 0),
        atom::make(C, 2, 0, 1, 0, 1, 0, 0),
        atom::make(C, 2, 1, 0, 1, 0, 0, 0),
        atom::make(C, 2, 0, 1, 0, 0, 0, 1),
    }},
    // CH3OH, Methanol
    temp_precursor{2, {
        0, 0, 1, 0
    }, {
        atom::make(C, 3, 1, 0, 0, 0, 0, 0),
        atom::make(O, 1, 0, 0, 0, 1, 0, 0),
    }},
    // C2H6O2, Ethylene Glycol
    temp_precursor{4, {
        0, -1, 0, 0, 1, 0, 1, 1,
    }, {
        atom::make(O, 1, 0, 1, 0, 0, 0, 0),
        atom::make(C, 2, 1, 0, 0, 0, 1, 0),
        atom::make(C, 2, 0, 1, 0, 1, 0, 0),
        atom::make(O, 1, 0, 0, 0, 0, 1, 0),
    }},
    // C3H6, Propene
    temp_precursor{3, {
        0, 0, 0, 1, 1, 1
    }, {
        atom::make(C, 3, 0, 1, 0, 0, 0, 0),
        atom::make(C, 1, 2, 0, 0, 0, 1, 0),
        atom::make(C, 2, 0, 0, 0, 2, 0, 0),
    }},
    // CH3COOH, Acetic Acid
    temp_precursor{4, {
        0, 0, 0, 1, -1, 0, 1, -1
    }, {
        atom::make(C, 0, 0, 2, 0, 1, 0, 1),
        atom::make(O, 0, 0, 0, 0, 0, 2, 0),
        atom::make(C, 3, 1, 0, 0, 0, 0, 0),
        atom::make(O, 1, 0, 0, 1, 0, 0, 0),
    }},
    // C6H6, Benzene
    temp_precursor{6, {
        0, 0, 1, 0, 2, -1, 2, -2, 1, -2, 0, -1
    }, {
        atom::make(C, 1, 2, 0, 0, 0, 1, 0),
        atom::make(C, 1, 0, 0, 0, 2, 0, 1),
        atom::make(C, 1, 0, 0, 1, 0, 2, 0),
        atom::make(C, 1, 0, 2, 0, 1, 0, 0),
        atom::make(C, 1, 1, 0, 2, 0, 0, 0),
        atom::make(C, 1, 0, 1, 0, 0, 0, 2),
    }},
    // CH4N2O, Carbamide
    temp_precursor{4, {
        0, 0, 0, 1, -1, 0, 1, -1
    }, {
        atom::make(C, 0, 0, 2, 0, 1, 0, 1),
        atom::make(O, 0, 0, 0, 0, 0, 2, 0),
        atom::make(N, 2, 1, 0, 0, 0, 0, 0),
        atom::make(N, 2, 0, 0, 1, 0, 0, 0),
    }},
    // H3PO4, Phosphoric Acid
    temp_precursor{5, {
        0, 0, 1, 0, -1, 1, -1, 0, 0, -1
    }, {
        atom::make(P, 0, 2, 0, 1, 1, 1, 0),
        atom::make(O, 0, 0, 0, 0, 2, 0, 0),
        atom::make(O, 1, 0, 0, 0, 0, 0, 1),
        atom::make(O, 1, 1, 0, 0, 0, 0, 0),
        atom::make(O, 1, 0, 1, 0, 0, 0, 0),
    }},
    // HF, Hydrofluoric Acid
    temp_precursor{1, {0, 0}, {atom::make(F, 1, 0, 0, 0, 0, 0, 0)}},
    // N2H4, Hydrazine
    temp_precursor{2, {
        0, 0, 1, 0
    }, {
        atom::make(N, 2, 1, 0, 0, 0, 0, 0),
        atom::make(N, 2, 0, 0, 0, 1, 0, 0),
    }},
    // C4H8O, Butanone
    temp_precursor{5, {
        0, 0, 0, 1, -1, 0, 1, -1, 2, -1
    }, {
        atom::make(C, 0, 0, 2, 0, 1, 0, 1),
        atom::make(O, 0, 0, 0, 0, 0, 2, 0),
        atom::make(C, 3, 1, 0, 0, 0, 0, 0),
        atom::make(C, 2, 1, 0, 1, 0, 0, 0),
        atom::make(C, 3, 0, 0, 0, 1, 0, 0),
    }},
    // SOCl2, Thionyl Chloride
    temp_precursor{4, {
        0, 0, 0, 1, -1, 0, 1, -1
    }, {
        atom::make(S, 0, 0, 2, 0, 1, 0, 1),
        atom::make(O, 0, 0, 0, 0, 0, 2, 0),
        atom::make(Cl, 0, 1, 0, 0, 0, 0, 0),
        atom::make(Cl, 0, 0, 0, 1, 0, 0, 0),
    }},
    // C4H8O2, Dioxane
    temp_precursor{6, {
        0, 0, 0, 1, 1, 1, 2, 0, 2, -1, 1, -1
    }, {
        atom::make(O, 0, 0, 1, 0, 0, 0, 1),
        atom::make(C, 2, 1, 0, 0, 0, 1, 0),
        atom::make(C, 2, 0, 0, 0, 1, 0, 1),
        atom::make(O, 0, 0, 0, 1, 0, 1, 0),
        atom::make(C, 2, 0, 1, 0, 1, 0, 0),
        atom::make(C, 2, 1, 0, 1, 0, 0, 0),
    }},
    // C7H8, Toluene
    temp_precursor{7, {
        0, 0, 0, 1, 1, 1, 2, 0, 2, -1, 1, -1, 3, 0
    }, {
        atom::make(C, 1, 0, 1, 0, 0, 0, 2),
        atom::make(C, 1, 2, 0, 0, 0, 1, 0),
        atom::make(C, 1, 0, 0, 0, 2, 0, 1),
        atom::make(C, 0, 1, 0, 1, 0, 2, 0),
        atom::make(C, 1, 0, 2, 0, 1, 0, 0),
        atom::make(C, 1, 1, 0, 2, 0, 0, 0),
        atom::make(C, 3, 0, 0, 0, 1, 0, 0),
    }},
    // CH2O2, Formic Acid
    temp_precursor{3, {
        0, -1, 0, 0, 1, 0
    }, {
        atom::make(O, 1, 0, 1, 0, 0, 0, 0),
        atom::make(C, 1, 2, 0, 0, 0, 1, 0),
        atom::make(O, 0, 0, 0, 0, 2, 0, 0),
    }},
    // C4H8, Butylene
    temp_precursor{4, {
        -1, 1, 0, 0, 1, 0, 2, -1
    }, {
        atom::make(C, 3, 0, 0, 0, 0, 0, 1),
        atom::make(C, 2, 1, 0, 1, 0, 0, 0),
        atom::make(C, 1, 0, 0, 0, 1, 0, 2),
        atom::make(C, 2, 0, 0, 2, 0, 0, 0),
    }},
    // C4H10, Isobutane
    temp_precursor{4, {
        0, 0, 0, 1, -1, 0, 1, -1
    }, {
        atom::make(C, 1, 0, 1, 0, 1, 0, 1),
        atom::make(C, 3, 0, 0, 0, 0, 1, 0),
        atom::make(C, 3, 1, 0, 0, 0, 0, 0),
        atom::make(C, 3, 0, 0, 1, 0, 0, 0),
    }},
    // C3H3N3, S-Triazine
    temp_precursor{6, {
        0, 0, 0, 1, 1, 1, 2, 0, 2, -1, 1, -1
    }, {
        atom::make(N, 0, 0, 1, 0, 0, 0, 2),
        atom::make(C, 1, 2, 0, 0, 0, 1, 0),
        atom::make(N, 0, 0, 0, 0, 2, 0, 1),
        atom::make(C, 1, 0, 0, 1, 0, 2, 0),
        atom::make(N, 0, 0, 2, 0, 1, 0, 0),
        atom::make(C, 1, 1, 0, 2, 0, 0, 0),
    }},
});

#endif
