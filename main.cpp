#include <iostream>
#include <stdlib.h>
#include <cstring>
#include "OFDM/modulation.hpp"
#include <vector>
#include <chrono>
#include <fstream>
#include <random>
#include "OFDM/Frame.hpp"
#include "sdr/sdr.hpp"
#include <thread>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <iterator>
#include "io.hpp"



int main(){
    
    FRAME_FORM tx_frame("config/config.txt");
    FRAME_FORM rx_frame("config/config.txt");

    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");
    SDR rx_sdr(1, tx_frame.output_size, "config/config.txt");

    
    bit_vector origin_mes(tx_frame.usefull_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

 
    for (size_t i = 0; i < origin_mes.size(); i++) {
        origin_mes[i] = dis(gen);
    }
    
    tx_frame.write(origin_mes);
    
    auto mod_data   = tx_frame.get();
    auto tx_data    = tx_frame.get_int16();
  

    write_complex_to_file("data/tx_data.bin", tx_data.begin(), tx_data.end());
    read_complex_from_file("data/tx_data.bin", rx_frame.rx_frame_int16_buf.begin());

    // tx_sdr.send(tx_data);
    // rx_sdr.recv(rx_frame.rx_frame_int16_buf);


    rx_frame.form_int16_to_double();
    auto symb_begin = rx_frame.t2sin.find_next_symb_with_t2sin(rx_frame.rx_frame_buf, 0);

    rx_frame.preamble.find_cor_with_preamble(rx_frame.rx_frame_buf, symb_begin);
    
    auto preamble_begin = rx_frame.preamble.find_start_symb_with_preamble(rx_frame.rx_frame_buf, symb_begin);
    
    auto frame_begin = symb_begin+preamble_begin-rx_frame.t2sin.size;
    
    memcpy(rx_frame.tx_frame_buf.data(), rx_frame.rx_frame_buf.data()+frame_begin, sizeof(complex_double)*(rx_frame.output_size));


    // double pilot_freq_shift = rx_frame.message_with_preamble.pilot_freq_shift();
    // rx_frame.message_with_preamble.freq_shift(pilot_freq_shift);
    // rx_frame.message_with_preamble.cp_freq_sinh();
    // rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.output[0], rx_frame.preamble.size);
    
    auto constell = rx_frame.message.fft();

    write_complex_to_file("data/data.bin", rx_frame.rx_frame_buf.begin(), rx_frame.rx_frame_buf.end());    
    write_double_to_file("data/cor.bin", rx_frame.preamble.cor);
    write_complex_to_file("data/frame.bin", rx_frame.tx_frame_buf.begin()+rx_frame.t2sin.size, rx_frame.tx_frame_buf.end());
    write_complex_to_file("data/constell.bin", constell.begin(), constell.end());

    
    return 0;
}


