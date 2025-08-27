#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <mkl.h>
#include "modulation.hpp"
#include "OFDM.hpp"
#include <vector>
#include <chrono>



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
    for (int i = 0; i < v.size(); ++i)
        std::cout<<v[i];
    std::cout<<"\n";
}


int main(){
    const char* SFILE_NAME = "text.txt";

    std::vector<uint8_t> buf(10240);
    Modulation Mod(qam16);
    FILE* SFILE = fopen(SFILE_NAME, "r");
    
    size_t file_len = fread(buf.data(), 1, buf.size(), SFILE);

    OFDM ofdm("config.txt");
        
    auto mod_data = Mod.mod(buf);
        
    auto demod_data = Mod.demod(mod_data);

    // auto avg_us = bench_us([&](){
    //         auto mod_data = Mod.mod(buf);
    //         auto out = Mod.demod(mod_data);
    //         volatile size_t sink = out.size();
    //         (void)sink;
    //         });
    //     std::cout << "avg time: " << avg_us << " us\n";
                
                

        
        // print_vector(buf);
        // print_vector(res);
        
    demod_data.resize(buf.size());
    std::cout<<(buf==demod_data)<<'\n';

    return 0;
}


