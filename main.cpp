#include <iostream>
#include <stdlib.h>
#include <cstring>
#include "modulation.hpp"
#include "OFDM.hpp"
#include <vector>
#include <chrono>
#include <fstream>
#include <random>
#include "Frame.hpp"
#include "sdr.hpp"
#include <thread>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <iterator>


template <typename Iter>
void write_complex_to_file(const std::string &filename, Iter begin, Iter end) {
    using Type = typename std::iterator_traits<Iter>::value_type::value_type; 

    std::ofstream fout(filename, std::ios::binary);
    if (!fout) {
        throw std::runtime_error("Cannot open file");
    }

    for (auto it = begin; it != end; ++it) {
        Type re = it->real();
        Type im = it->imag();
        fout.write(reinterpret_cast<const char*>(&re), sizeof(Type));
        fout.write(reinterpret_cast<const char*>(&im), sizeof(Type));
    }

    fout.close();
}


void write_double_to_file(const std::string &filename, const std::vector<double> &data) {
    std::ofstream fout(filename, std::ios::binary);
    if (!fout) {
        throw std::runtime_error("Cannot open file");
    }
    fout.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(double));
    fout.close();
}


void send_data(const char* pipe, const complex16_vector& buf) {
    int fd = open(pipe, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {

        return;
    }

    for (auto& s : buf) {
        int16_t data[2];
        data[0] = s.real();
        data[1] = s.imag(); 

        ssize_t written = write(fd, data, sizeof(data));
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Pipe временно заполнен, пропускаем этот сэмпл
                continue;
            } else {
                perror("write pipe");
                break;
            }
        }
    }

    close(fd);
}


template<typename F>
long long bench_us(F&& f, int warmup = 5, int iters = 10000) {

    for (int i = 0; i < warmup; ++i) f();

    long long total_us = 0;
    for (int i = 0; i < iters; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        f();
        auto t1 = std::chrono::steady_clock::now();
        total_us += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    }
    return total_us / iters;
}


void print_vector(std::vector<uint8_t>& v){
    for (auto &i : v)
        std::cout<<i;
    std::cout<<"\n";
}


int main(){
    
    FRAME_FORM frame("config.txt");

    bit_vector origin_mes(frame.usefull_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 1);

    for (size_t i = 0; i < origin_mes.size(); i++) {
        origin_mes[i] = dis(gen);
    }

    // const char* SFILE_NAME = "text.txt";
    // FILE* SFILE = fopen(SFILE_NAME, "r");
    // fread(origin_mes.data(), 1, origin_mes.size(), SFILE);    
    // fclose(SFILE);

    
    frame.write(origin_mes);
    
    auto mod_data   = frame.get();
    auto tx_data    = frame.get_int16();

    // auto res_mes    = frame.read(mod_data.data()); 
    // res_mes.resize(origin_mes.size());
    // std::cout<<(origin_mes==res_mes)<<'\n';

    write_complex_to_file("tx.bin", tx_data.begin(), tx_data.end());
        
    SDR tx_sdr(0, frame.output_size, "config.txt");
    SDR rx_sdr(1, frame.output_size, "config.txt");

    complex16_vector rx_data(frame.output_size);
    
    tx_sdr.send(tx_data);
    rx_sdr.recv(frame.rx_frame_int16_buf);
        
    frame.form_int16_to_double();

    
    // volatile int guard = 0;
    // auto avg_us = bench_us([&]() {
    //     guard = frame.t2sin.find_next_symb(frame.rx_frame_buf, 0, 0.02);
    // });
    // std::cout << "Average execution time: " << avg_us << " us\n";

    // std::cout<<"symbol:"<<frame.t2sin.find_next_symb(frame.rx_frame_buf, 0, 0.02);
    
    auto symb_begin = frame.t2sin.find_next_symb_with_t2sin(frame.rx_frame_buf, 0, 0.02);
    frame.preamble.find_start_symb_with_preamble(frame.rx_frame_buf, symb_begin, 0.01);
    // std::cout<<frame.preamble.cor.size()<<" "<<frame.output_size<<"\n";

    volatile double dummy_sink = 0.0;

    auto avg_us = bench_us([&]() {
        frame.preamble.find_start_symb_with_preamble(frame.rx_frame_buf, symb_begin, 0.01);

        // используем результат, чтобы не выкинуло
        if (!frame.preamble.cor.empty()) {
            dummy_sink += frame.preamble.cor[0];
        }
    });

    std::cout<<avg_us<<" us"<<"\n";
    
    write_complex_to_file("frame.bin", frame.rx_frame_buf.begin()+symb_begin, frame.rx_frame_buf.begin()+symb_begin+frame.output_size);
    write_double_to_file("preamble_cor.bin", frame.preamble.cor);


    return 0;
}


