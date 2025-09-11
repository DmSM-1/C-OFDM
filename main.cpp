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


void write_complex_to_pipe(const char* pipe_path,
                           std::vector<std::complex<double>>::iterator begin,
                           std::vector<std::complex<double>>::iterator end)
{
    // Открываем pipe вручную через open с O_WRONLY | O_NONBLOCK
    int fd = open(pipe_path, O_WRONLY | O_NONBLOCK);
    if (fd < 0) throw std::runtime_error("Cannot open pipe");

    for (auto it = begin; it != end; ++it) {
        double data[2] = { it->real(), it->imag() };
        ssize_t written = write(fd, data, sizeof(data));
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            else { perror("write"); break; }
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
    
    FRAME_FORM tx_frame("config/config.txt");
    FRAME_FORM rx_frame("config/config.txt");

    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");
    SDR rx_sdr(1, tx_frame.output_size, "config/config.txt");

    
    bit_vector origin_mes(tx_frame.usefull_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 1);

    for (size_t i = 0; i < origin_mes.size(); i++) {
        origin_mes[i] = dis(gen);
    }
    
    tx_frame.write(origin_mes);
    
    auto mod_data   = tx_frame.get();
    auto tx_data    = tx_frame.get_int16();

    write_complex_to_file("tx.bin", tx_data.begin(), tx_data.end());  

    complex16_vector rx_data(tx_frame.output_size);
    
    tx_sdr.send(tx_data);
    rx_sdr.recv(rx_frame.rx_frame_int16_buf);

    rx_frame.form_int16_to_double();
    auto symb_begin = rx_frame.t2sin.find_next_symb_with_t2sin(rx_frame.rx_frame_buf, 0);
    auto preamble_begin = rx_frame.preamble.find_start_symb_with_preamble(rx_frame.rx_frame_buf, symb_begin);
    auto frame_begin = symb_begin+preamble_begin-rx_frame.t2sin.size;

    memcpy(rx_frame.rx_message_with_preamble_buf.data(),rx_frame.rx_frame_buf.data()+symb_begin+preamble_begin, sizeof(complex_double)*(rx_frame.output_size-rx_frame.t2sin.size));
    write_complex_to_file("data/frame.bin", rx_frame.rx_message_with_preamble_buf.begin(), rx_frame.rx_message_with_preamble_buf.end());
        

    return 0;
}


