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
#include <Python.h>
#include "mac/mac_frame.hpp"


int main(){
    
    FRAME_FORM tx_frame("config/config.txt");
    FRAME_FORM rx_frame("config/config.txt");

    MAC mac(1, 0, rx_frame.usefull_size);
    
    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");
    SDR rx_sdr(1, rx_frame.output_size, "config/config.txt");


    bit_vector origin_mes(mac.payload);
    FILE* file = fopen("WARANDPEACE.txt", "r");
    fread(origin_mes.data(), 1, origin_mes.size(), file);
    fclose(file);

    auto tx_mac_frame = mac.write(origin_mes, 0);

    tx_frame.write(tx_mac_frame);
    
    auto mod_data   = tx_frame.get();
    auto tx_data    = tx_frame.get_int16();  

    tx_sdr.send(tx_data);
    tx_sdr.send(tx_data);
    rx_sdr.recv(rx_frame.from_sdr_int16_buf);
    
    rx_frame.form_int16_to_double();
    
    auto t2_sin_corr = rx_frame.t2sin.corr(rx_frame.from_sdr_buf);
    auto t2_sin_begin = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, 0);
    
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
    
    write_complex_to_file("data/source.bin", tx_frame.int16_buf);
    write_complex_to_file("data/data.bin", rx_frame.from_sdr_buf);
    write_double_to_file("data/t2_sin_corr.bin", t2_sin_corr);
    write_complex_to_file("data/phases.bin", chan_char);
    write_complex_to_file("data/constell.bin", constell);

    auto res_ofdm = rx_frame.message.Mod.demod(constell);

    auto res = mac.read(res_ofdm);

    double acc = 0.0;
    for (int i = 0; i < rx_frame.usefull_size; i++){
        acc += double(res[i]==origin_mes[i]);
    }
    acc /= rx_frame.usefull_size;
    
    std::cout<<"FRAME FROM "<<mac.input_tx_id<<" TO "<<mac.input_rx_id<<" SEQ "<<mac.input_seq_num<<'\n';
    std::cout<<"ACCURACY: "<< acc <<"\n";

    acc = 0.0;
    for (int i = 0; i < rx_frame.usefull_size; i++) {
        uint8_t diff = res[i] ^ origin_mes[i];
        for (int b = 0; b < 8; b++) {
            acc += ((diff >> b) & 1) == 0;
        }
    }
    acc /= (rx_frame.usefull_size * 8);

    std::cout << "Bit-level ACCURACY: " << acc << "\n";

    FILE* res_file = fopen("data.txt", "w");
    fwrite(res.data(), 1, res.size(), res_file);
    fclose(res_file);

    system("python3 python_code/ofdm.py");
    
    return 0;
}


