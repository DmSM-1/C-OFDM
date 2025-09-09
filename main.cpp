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


void write_complex_to_file(const std::string &filename, const complex_vector &data) {
    std::ofstream fout(filename, std::ios::binary);
    if (!fout) {
        throw std::runtime_error("Cannot open file");
    }
    size_t len = data.size();
    for (size_t i = 0; i < len; ++i) {
        double re = data[i].real();
        double im = data[i].imag();
        fout.write(reinterpret_cast<const char*>(&re), sizeof(double));
        fout.write(reinterpret_cast<const char*>(&im), sizeof(double));
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


void write_complex_to_file(const std::string &filename, const complex16_vector &data) {
    std::ofstream fout(filename, std::ios::binary);
    if (!fout) {
        throw std::runtime_error("Cannot open file");
    }
    size_t len = data.size();
    for (size_t i = 0; i < len; ++i) {
        double re = (double)data[i].real();
        double im = (double)data[i].imag();
        fout.write(reinterpret_cast<const char*>(&re), sizeof(double));
        fout.write(reinterpret_cast<const char*>(&im), sizeof(double));
    }
    fout.close();
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
    auto res_mes    = frame.read(mod_data.data()); 
    
    res_mes.resize(origin_mes.size());
    std::cout<<(origin_mes==res_mes)<<'\n';
    
    // print_vector(origin_mes);
    // print_vector(res_mes);
    
    // long long avg_us = bench_us([&]() {
        //     frame.write(origin_mes);
        //     auto res = frame.read(frame.get().data()); 
        //     volatile auto guard = res.data();
        // });
        
        // std::cout<<avg_us<<"\n";
        
        // write_complex_to_file("data.bin", frame.get_int16());
        
    SDR tx_sdr(0, frame.output_size, "config.txt");
    SDR rx_sdr(1, frame.output_size, "config.txt");

    
    complex16_vector rx_data(frame.output_size);
    
    tx_sdr.send(tx_data);
    rx_sdr.recv(frame.rx_frame_int16_buf);
        
    // std::cout<<frame.find_t2sin()<<'\n';
    frame.form_int16_to_double();
    auto sin = frame.t2sin.find(frame.rx_frame_buf);
    
    write_complex_to_file("data.bin", frame.rx_frame_buf);
    write_double_to_file("sin.bin", sin);
    // auto sin = frame.find_t2sin();

    
    // for (int i = 0; i < 10000; i++){
    //     tx_sdr.send(tx_data);
    //     rx_sdr.recv(rx_data);
    //     // send_data("/tmp/row_input", rx_data);
    // }


    return 0;
}


