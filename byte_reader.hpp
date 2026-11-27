#include <array>
#include <cstdint>
#include <fstream>
class byte_reader {
    std::ifstream is;

    template<std::streamsize N>
    std::array<char, N> read_bytes() {
        std::array<char, N> buf{};
        is.read(buf.data(), N);
        if (is.gcount() != N) {throw std::runtime_error("Unexpected EOF");}
        return buf;
    }
public:
    byte_reader(const char* path): is{path, std::ios::binary} {};
    int32_t read_int32_le() {
        auto buf = read_bytes<4>();
        return (int32_t) (
            (uint32_t)buf[3] << 24 |
            (uint32_t)buf[2] << 16 |
            (uint32_t)buf[1] << 8 |
            (uint32_t)buf[0]
        );
    }

    uint8_t read_byte() {
        auto buf = read_bytes<1>();
        return (uint8_t)buf[0];
    }

    bool read_bool() {
        auto buf = read_bytes<1>();
        return buf[0] != 0;
    }

    void skip_string() {
        int32_t len = read_int32_le();
        is.seekg(len, std::ios::cur);
    }
};
