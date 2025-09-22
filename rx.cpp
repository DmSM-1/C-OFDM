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
#include "io/io.hpp"
#include "mac/mac_frame.hpp"


int main(){
    
    FRAME_FORM tx_frame("config/config.txt");
    MAC mac(1, 0, tx_frame.usefull_size);
    SDR rx_sdr(1, tx_frame.output_size, "config/config.txt");

    for(int i = 0; i < 1000000; i++){

        FRAME_FORM rx_frame("config/config.txt");


        rx_sdr.recv(rx_frame.from_sdr_int16_buf);
        
        rx_frame.form_int16_to_double();
        
        auto t2_sin_corr = rx_frame.t2sin.corr(rx_frame.from_sdr_buf);
        auto t2_sin_begin = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, 0);

        std::cout<<t2_sin_begin<<"\n";
        if (t2_sin_begin == -1)
            continue;
        
        auto pr_begin = rx_frame.preamble.find_preamble(rx_frame.from_sdr_buf, t2_sin_begin)+1;

        std::copy(
            rx_frame.from_sdr_buf.begin()+t2_sin_begin+pr_begin-rx_frame.t2sin.size, 
            rx_frame.from_sdr_buf.begin()+t2_sin_begin+pr_begin-rx_frame.t2sin.size+rx_frame.output_size, 
            rx_frame.buf.begin());
        
        auto freq_shift = rx_frame.message_with_preamble.pilot_freq_sinh();
        rx_frame.message_with_preamble.freq_shift(freq_shift);
        rx_frame.message_with_preamble.cp_freq_sinh();
        rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.ofdm_preamble.data(), rx_frame.preamble.size);

        auto chan_char = rx_frame.preamble.chan_char_lq();
        auto constell = rx_frame.message.fft();
        
        for (int i = 0; i < constell.size(); i++){
            constell[i] /= chan_char[i%chan_char.size()];
        }

        write_complex_to_file("data/data.bin", rx_frame.buf);
        write_double_to_file("data/t2_sin_corr.bin", t2_sin_corr);
        write_complex_to_file("data/phases.bin", chan_char);
        write_complex_to_file("data/constell.bin", constell);

        auto res_ofdm = rx_frame.message.Mod.demod(constell);
        auto res = mac.read(res_ofdm);
        
        std::cout<<"FRAME FROM "<<mac.input_tx_id<<" TO "<<mac.input_rx_id<<" SEQ "<<mac.input_seq_num<<'\n';

        char filename[64] = {0};
        sprintf(filename, "frames/rx_frame_%d.txt", mac.seq_num);
        FILE* res_file = fopen(filename, "w");
        fwrite(res.data(), 1, res.size(), res_file);
        fclose(res_file);

        system("python3 python_code/ofdm.py");

    }
    
    return 0;
}


