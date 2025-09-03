#include <stdlib.h>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>
#include "modulation.hpp"
#include "parcer.hpp"
#include <fftw3.h>
#include <cstring>

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <random>


const complex_double REAL_ONE(1.0, 0.0);



class FFT_FORM{
public:
    int fft_size;
    int num_data_subc;
    int num_pilot_subc;

    int num_symb;

    int segment_step;
    int segment_size;
    int segment_byte_size;

    complex_vector FFT_buf;
    std::vector<complex_double*> segment;
    std::vector<complex_double*> pilot;

    fftw_plan backward_plan;
    fftw_plan forward_plan;

    complex_vector restored_buf;
    
    double norm_factor;

    FFT_FORM(int fft_size,int num_data_subc,int num_pilot_subc, int num_symb);
    void write(complex_vector& input);
    complex_vector& read();
};


class T2SIN_FORM{

private:
    ConfigMap& config;
    
public:
    complex_double* buf = nullptr;
    int size;
    int f1;
    int f2;
    
    T2SIN_FORM(ConfigMap& config);
    void set(complex_double* buf_ptr);

};


class OFDM_FORM{

private:
    ConfigMap& config;

public:
    bool data;

    int fft_size;
    int num_data_subc;
    int num_pilot_subc;
    int cp_size;
    int num_symb;

    int pr_sin_len;
    int pr_seed;
    mod_type modType;

    int size;
    int usefull_size;

    std::vector<complex_double*> output;

    FFT_FORM fft_task;

    Modulation Mod;

    int byte_fft_size;

    OFDM_FORM(ConfigMap& config, bool data = true);
    virtual void set(complex_double* buf_ptr);
    void write(bit_vector& input);
    bit_vector read();
};


class PREAMBLE_FORM : public OFDM_FORM{

public:
    bit_vector preamble;
    complex_vector mod_preamble;

    PREAMBLE_FORM(ConfigMap& config);
    void set(complex_double* buf_ptr);

};


class FRAME_FORM{

private:
    ConfigMap config;

    T2SIN_FORM      t2sin;
    PREAMBLE_FORM   preamble;
    OFDM_FORM       message;
    
    complex_vector frame_buf;

public:

    int usefull_size;

    bit_vector bit_preambple;

    FRAME_FORM(const std::string& CONFIGNAME);
    void write(bit_vector& input);
    
    bit_vector read(void* transmitted_data);

    complex_vector get();
    

};