#include <stdlib.h>
#include <vector>
#include <math.h>
#include <utility>
#include <cstring>

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <random>
#include <modulation.hpp>


class L2_frame{
    
    public:
        int L1_size;
        int L2_size;
        bit_vector L1_buf;
        bit_vector L2_buf;
        
        bool cor = false;

        const int code_len = 5;
        const int tx_id = 1;
        const int rx_id = 1;
        const int fcs = 1;
        const int seq_num = 1;
        const uint8_t one = 22;
        const uint8_t zero = 9;

        int usefull_size;;


    L2_frame(int L1_size)
    :   L1_size(L1_size),
        L2_size(L1_size/code_len),
        L1_buf(L1_size, 0),
        L2_buf(L2_size, 0),
        usefull_size(L2_size - tx_id - rx_id - fcs - seq_num)
    {}

    void encode() {
        for (int i = 0; i < L2_size; i++) {
            uint8_t code = L2_buf[i] ? one : zero;
            for (int b = 0; b < code_len; b++) {
                L1_buf[i * code_len + (code_len - 1 - b)] = (code >> b) & 1;
            }
        }
    }

    static int hamming(uint8_t a, uint8_t b) {
        return __builtin_popcount(a ^ b);
    }

    void decode() {
        for (int i = 0; i < L2_size; i++) {
            uint8_t code = 0;
            for (int b = 0; b < code_len; b++) {
                code = (code << 1) | L1_buf[i * code_len + b];
            }

            int dist_one  = hamming(code, one);
            int dist_zero = hamming(code, zero);

            if (dist_one < dist_zero) {
                L2_buf[i] = 1;
            } else{
                L2_buf[i] = 0;
        }
    }
};


void write(uint8_t tx, uint8_t rx, uint8_t seq, uint8_t* data){
}
    L2_buf[0]
};