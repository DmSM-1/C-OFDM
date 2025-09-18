#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <chrono>
#include <fstream>
#include <random>
#include <thread>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <iterator>
#include <Python.h>


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

template <typename T>
void write_complex_to_file(const std::string &filename, const std::vector<std::complex<T>> &data) {
    using Type = typename std::vector<std::complex<T>>::value_type::value_type; 

    std::ofstream fout(filename, std::ios::binary);
    if (!fout) {
        throw std::runtime_error("Cannot open file");
    }

    for (auto &it : data) {
        Type re = it.real();
        Type im = it.imag();
        fout.write(reinterpret_cast<const char*>(&re), sizeof(Type));
        fout.write(reinterpret_cast<const char*>(&im), sizeof(Type));
    }

    fout.close();
}

template <typename Iter>
void read_complex_from_file(const std::string &filename, Iter out) {
    using Type = typename std::iterator_traits<Iter>::value_type::value_type; // тип числа (float, double, ...)

    std::ifstream fin(filename, std::ios::binary);
    if (!fin) {
        throw std::runtime_error("Cannot open file");
    }

    Type re, im;
    while (fin.read(reinterpret_cast<char*>(&re), sizeof(Type))) {
        if (!fin.read(reinterpret_cast<char*>(&im), sizeof(Type))) {
            throw std::runtime_error("File corrupted: incomplete complex number");
        }
        *out++ = std::complex<Type>(re, im);
    }
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



template <typename Iter>
void write_complex_to_pipe(Iter begin, Iter end, const char* pipe_name) {
    std::ofstream pipe(pipe_name, std::ios::binary);
    for (Iter it = begin; it != end; ++it) {
        double re = it->real();
        double im = it->imag();
        pipe.write(reinterpret_cast<char*>(&re), sizeof(double));
        pipe.write(reinterpret_cast<char*>(&im), sizeof(double));
    }
    pipe.close();
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


void print_acc(const int size, const bit_vector& source, const bit_vector& result, const char* filename = "stat.txt"){

    FILE* file = fopen(filename, "a");

    double acc = 0.0;
    for (int i = 0; i < size; i++){
        acc += double(source[i]==result[i]);
    }
    acc /= size;
    
    fprintf(file, "ACCURACY: %.6lf ", acc);

    acc = 0.0;
    for (int i = 0; i < size; i++) {
        uint8_t diff = result[i] ^ source[i];
        for (int b = 0; b < 8; b++) {
            acc += ((diff >> b) & 1) == 0;
        }
    }
    acc /= (size * 8);

    fprintf(file, "Bit-level ACCURACY: %3.6lf SIZE: %.3lf KB\n", acc, (double)size/1024);

    fclose(file);
}