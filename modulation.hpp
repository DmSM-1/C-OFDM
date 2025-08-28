#pragma once
#include <stdlib.h>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>
#include <algorithm>
#include <immintrin.h>


enum mod_type {
    bpsk = 1, 
    qam4 = 2, 
    qam16 = 4,
    qam64 = 6,
    qam256 = 8
};


using complex_double = std::complex<double>;
using complex_vector = std::vector<std::complex<double>>;

complex_double psk(uint8_t input, double angle, int deg);
complex_double qam(uint8_t input, int deg);


class Modulation{
    private:
        std::vector<uint8_t> demod_buffer;
        uint8_t str_size;
        double str_size_1;
        double step;

    public:
        mod_type modulation;
        std::vector<complex_double> constell;
        size_t mod_index;

        Modulation(mod_type mod);

        complex_vector mod(std::vector<uint8_t>& bin_input);
        std::vector<uint8_t> demod(complex_vector& input);
        std::vector<uint8_t> bit_stream_converter(size_t output_block_size, size_t input_block_size, std::vector<uint8_t>& input);

};