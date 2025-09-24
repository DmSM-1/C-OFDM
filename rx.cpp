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
    
    FRAME_FORM rx_frame("config/config.txt");
    MAC mac(1, 0, rx_frame.usefull_size);
    SDR rx_sdr(1, rx_frame.output_size, "config/config.txt");

    // rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);

    rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
    rx_frame.form_int16_to_double();

    char stat[64] = "data/stat.txt";
    FILE* stat_file = fopen(stat, "w");

    int pos = 0;
    int preamble_begin;
    int threshold = rx_frame.from_sdr_buf.size()-rx_frame.output_size;

    for(int i = 0; i < 10000; i++){
        
        pos = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, pos);

        if (pos == -1){
            // std::cout<<"empty "<<i<<"\n";
            pos = rx_frame.output_size;
            rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            rx_frame.form_int16_to_double();
            continue;
        }

        if (pos >= threshold){
            // std::cout<<"refresh "<<i<<"\n";
            std::copy(rx_frame.from_sdr_int16_buf.begin()+threshold, 
            rx_frame.from_sdr_int16_buf.end(), 
            rx_frame.from_sdr_int16_buf.begin());

            rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            rx_frame.form_int16_to_double();

            pos -= threshold;
        }

        preamble_begin = rx_frame.preamble.find_preamble(rx_frame.from_sdr_buf, pos)+1;

        if (preamble_begin < -2){
            // printf("detect error %d\n", i);
            pos += rx_frame.message.size;
            continue;
        }
        pos = preamble_begin;
        
        if (pos == -1){
            // std::cout<<"empty "<<i<<"\n";
            pos = rx_frame.output_size;
            rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            rx_frame.form_int16_to_double();
            continue;
        }

        if (pos >= threshold+rx_frame.t2sin.size){
            std::copy(rx_frame.from_sdr_int16_buf.begin()+threshold, 
            rx_frame.from_sdr_int16_buf.end(), 
            rx_frame.from_sdr_int16_buf.begin());

            rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            rx_frame.form_int16_to_double();

            pos -= threshold;
        }

        std::copy(
            rx_frame.from_sdr_buf.begin()+pos, 
            rx_frame.from_sdr_buf.begin()+pos-rx_frame.t2sin.size+rx_frame.output_size, 
            rx_frame.buf.begin()+rx_frame.t2sin.size);

        pos += rx_frame.message.size;
        
        auto freq_shift = rx_frame.message_with_preamble.pilot_freq_sinh();
        rx_frame.message_with_preamble.freq_shift(freq_shift);
        rx_frame.message_with_preamble.cp_freq_sinh();
        rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.ofdm_preamble.data(), rx_frame.preamble.size);

        auto chan_char = rx_frame.preamble.chan_char_lq();
        auto constell = rx_frame.message.fft();
        
        for (int j = 0; j < constell.size(); j++){
            constell[j] /= chan_char[j%chan_char.size()];
        }

        // if (i == 999){
        //     write_complex_to_file("data/data.bin", rx_frame.from_sdr_buf);
        //     write_complex_to_file("data/row_data.bin", rx_frame.buf);
        //     write_complex_to_file("data/phases.bin", chan_char);
        //     write_complex_to_file("data/constell.bin", constell);
        //     system("python3 python_code/ofdm.py");
        // }

        auto res_ofdm = rx_frame.message.Mod.demod(constell);
        auto res = mac.read(res_ofdm);
        

        char filename[64] = {0};
        sprintf(filename, "frames/rx_frame_%d.txt", mac.input_seq_num);
        FILE* res_file = fopen(filename, "w");
        fwrite(res.data(), 1, res.size(), res_file);
        fclose(res_file);

        
        char tx_filename[64];
        sprintf(tx_filename, "frames/frame_%d.txt", mac.input_seq_num);
        std::ifstream tx_file(tx_filename, std::ios::binary);
        if (!tx_file) {
            printf("FRAME FROM %3d TO %3d SEQ_NUM %5d ITER %6d POS %6d\n", mac.input_tx_id, mac.input_rx_id, mac.input_seq_num, i, pos);
            continue;
        }
        bit_vector origin_mes((std::istreambuf_iterator<char>(tx_file)),
        std::istreambuf_iterator<char>());
        tx_file.close();
        
        size_t sz = std::min(res.size(), origin_mes.size());
        double acc = 0.0;
        for (size_t i = 0; i < sz; i++) {
            if (res[i] == origin_mes[i])
            acc += 1.0;
        }
        acc /= sz;
        
        
        double bit_acc = 0.0;
        for (int i = 0; i < sz; i++) {
            uint8_t diff = res[i] ^ origin_mes[i];
            for (int b = 0; b < 8; b++) {
                bit_acc += ((diff >> b) & 1) == 0;
            }
            
        }
        bit_acc /= sz*8;
            
        printf("FRAME FROM %3d TO %3d SEQ_NUM %5d ACCURACY %.5lf %.5lf ITER %6d\n", mac.input_tx_id, mac.input_rx_id, mac.input_seq_num, acc, bit_acc, i, pos);
        fprintf(stat_file ,"FRAME FROM %3d TO %3d SEQ_NUM %5d ACCURACY %.5lf %.5lf ITER %6d\n", mac.input_tx_id, mac.input_rx_id, mac.input_seq_num, acc, bit_acc, i);
    }

    fclose(stat_file);
    printf("\n");
    
    return 0;
}


