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



template<typename F>
long long bench_us(F&& f, int warmup = 5, int iters = 10000) {
    // прогрев (не считаем)
    for (int i = 0; i < warmup; ++i) f();

    // измерение
    long long total_us = 0;
    for (int i = 0; i < iters; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        f();  // один запуск
        auto t1 = std::chrono::steady_clock::now();
        total_us += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    }
    return total_us / iters; // среднее время в мкс
}


void print_vector(std::vector<uint8_t>& v){
    for (auto &i : v)
        std::cout<<i;
    std::cout<<"\n";
}


int main(){
    
    std::vector<uint8_t> buf(10240);
    
    const char* SFILE_NAME = "text.txt";
    FILE* SFILE = fopen(SFILE_NAME, "r");
    fread(buf.data(), 1, buf.size(), SFILE);
    
    fclose(SFILE);

    FRAME_FORM frame("config.txt");
    buf.resize(frame.usefull_size);
    frame.write(buf);

    auto trans = frame.get();
    auto for_tx = frame.get_int16();
    auto res = frame.read(trans.data()); 

    // print_vector(buf);
    // print_vector(res);

    // long long avg_us = bench_us([&]() {
    // auto res = frame.read(trans.data()); 
    // // защита от оптимизаций
    // volatile auto guard = res.data();
    // });

    // std::cout<<avg_us<<"\n";



    write_complex_to_file("data.bin", frame.get());

        
    print_vector(buf);
    print_vector(res);
        
    res.resize(buf.size());
    std::cout<<(buf==res)<<'\n';

    
    SDR sdr(0, TX, frame.output_size);

    while (true)
    {
        sdr.send(for_tx);
    }
    

    return 0;
}


