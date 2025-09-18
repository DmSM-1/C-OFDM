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


int main(){
    
    FRAME_FORM tx_frame("config/config.txt");
    FRAME_FORM rx_frame("config/config.txt");

    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");
    SDR rx_sdr(1, tx_frame.output_size, "config/config.txt");


    bit_vector origin_mes(tx_frame.usefull_size);
    FILE* file = fopen("WARANDPEACE.txt", "r");
    fread(origin_mes.data(), 1, origin_mes.size(), file);
    fclose(file);

    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> dis(0, 255);
 
    // for (size_t i = 0; i < origin_mes.size(); i++) {
    //     origin_mes[i] = dis(gen);
    // }
    
    tx_frame.write(origin_mes);
    
    auto mod_data   = tx_frame.get();
    auto tx_data    = tx_frame.get_int16();
  
    tx_sdr.send(tx_data);

    int begin = 0;
    int window_size = rx_frame.from_sdr_buf.size()-rx_frame.output_size;

    rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
    rx_frame.form_int16_to_double();
    
    for (int l = 0; l < 10; l++){

        printf("\r");
        printf("Iter %6d", l);
        
        auto t2_sin_corr = rx_frame.t2sin.corr(rx_frame.from_sdr_buf);
        auto t2_sin_begin = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, begin);

        // std::cout<<"T2sin BEGIN: "<<t2_sin_begin<<'\n';

        if (t2_sin_begin == -1){
            rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            rx_frame.form_int16_to_double();
            begin = rx_frame.output_size;
            continue;
        }

        
        rx_frame.preamble.find_corr(rx_frame.from_sdr_buf, t2_sin_begin);
        auto pr_begin = rx_frame.preamble.find_preamble(rx_frame.from_sdr_buf, t2_sin_begin)+1;
        begin = t2_sin_begin+pr_begin-rx_frame.t2sin.size;
        
        // std::cout<<"BEGIN: "<<begin<<" "<<window_size<<'\n';

        if (begin > window_size){
            rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            memcpy(rx_frame.from_sdr_int16_buf.data(), rx_frame.from_sdr_int16_buf.data()+window_size, rx_frame.output_size*sizeof(std::complex<int16_t>));
            rx_frame.form_int16_to_double();
            begin -= window_size;
        }

        
        write_double_to_file("data/pr_corr.bin", rx_frame.preamble.cor);
        
        std::copy(
            rx_frame.from_sdr_buf.begin()+begin, 
            rx_frame.from_sdr_buf.begin()+begin+rx_frame.output_size, 
            rx_frame.buf.begin());
            
        begin += rx_frame.output_size-100;
        
        rx_frame.message_with_preamble.pilot_freq_sinh();
        rx_frame.message_with_preamble.cp_freq_sinh();
        rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.ofdm_preamble.data(), rx_frame.preamble.size);

        
        auto constell = rx_frame.message.fft();
        
        auto phases = rx_frame.preamble.phase_shift();
        
        for (int i = 0; i < constell.size(); i++){
            constell[i] /= phases[i%phases.size()];
        }
        
        auto res_data = rx_frame.message.fft();
        
        for (int i = 0; i < res_data.size(); i++){
            res_data[i] /= phases[i%phases.size()];
        }
        
        auto res = rx_frame.message.Mod.demod(res_data);
        res.resize(rx_frame.usefull_size);
        
        print_acc(rx_frame.usefull_size, origin_mes, res, "stat1.txt");
        
        FILE* res_file = fopen("data.txt", "w");
        fwrite(res.data(), 1, res.size(), res_file);
        fclose(res_file);
        

        write_complex_to_file("data/data.bin", rx_frame.from_sdr_buf);
        write_double_to_file("data/t2_sin_corr.bin", t2_sin_corr);
        write_complex_to_file("data/row.bin", rx_frame.buf.begin()+rx_frame.t2sin.size, rx_frame.buf.end());
        write_complex_to_file("data/phases.bin", phases);
        write_complex_to_file("data/constell.bin", constell);
        
        // system("python3 python_code/ofdm.py");
        // Py_Initialize();
        // PyRun_SimpleString(
        // );   
        // Py_Finalize();
        // system("python3 python_code/ofdm.py");

    }


    return 0;
}

