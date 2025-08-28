#include <iostream>
#include <stdlib.h>
#include <cstring>
#include "modulation.hpp"
#include "OFDM.hpp"
#include <vector>
#include <chrono>
#include <fstream>
#include <random>


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
long long bench_us(F&& f, int warmup = 5, int iters = 1000) {
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
    
    // for (auto &i: buf)
    // i = std::rand()%256;
    
    // Modulation Mod(qam256);
    
    const char* SFILE_NAME = "text.txt";
    FILE* SFILE = fopen(SFILE_NAME, "r");
    size_t file_len = fread(buf.data(), 1, buf.size(), SFILE);
    fclose(SFILE);

    OFDM ofdm("config.txt");
    ofdm.mod(buf);
    auto res = ofdm.demod();
    
    // complex_vector v(ofdm.full_len, 0);
    // memcpy(v.data(), ofdm.ofdm_buf, v.size()*sizeof(complex_double));

    // write_complex_to_file("data", v);
        
    // auto demod_data = Mod.demod(mod_data);

    // auto avg_us = bench_us([&](){
    //         auto inter = Mod.mod(buf);
    //         auto out = Mod.demod(mod_data);
    //         volatile size_t sink = out.size();
    //         (void)sink;
    //         });
    //     std::cout << "avg time: " << avg_us << " us\n";
                
                

        
    print_vector(buf);
    print_vector(res);
        
    // demod_data.resize(buf.size());
    std::cout<<(buf==res)<<'\n';

    return 0;
}


