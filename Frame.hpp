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
    int size;
    int f1;
    int f2;

    std::vector<double> detect_mask;
    complex_vector detect_buf;

    fftw_plan detect_plan;

    complex_double* buf = nullptr;


    T2SIN_FORM(ConfigMap& config);
    void set(complex_double* buf_ptr);

    std::vector<double> find(complex_vector& signal, double level = 0.01){

        
        int cycles      = signal.size()/size;

        std::vector<double> res(cycles, 0.0);

        auto signal_ptr = signal.data();
        auto buf_ptr    = detect_buf.data();
        
        double total_energy = 0.0;
        double sin_energy   = 0.0;
        double subc_energy  = 0.0;
        double re           = 0.0;
        double im           = 0.0;
        double rel          = 0.0;
        
        for(int i = 0; i < cycles; i++, signal_ptr+=size){
            
            total_energy    = 0.0;
            sin_energy      = 0.0;
            
            memcpy(buf_ptr, signal_ptr, size*sizeof(complex_double));
            fftw_execute(detect_plan);

            for (int j = 0; j < size; j++){
                re = detect_buf[j].real();
                im = detect_buf[j].imag();

                subc_energy = re*re+im*im;

                total_energy += subc_energy;
                sin_energy += detect_mask[j]*subc_energy;
            }
            
            if (total_energy == 0)
                continue;
            
            rel = sin_energy/total_energy;

            if (std::isnan(rel))
                continue;

            res[i] = rel;
            
            
            // if (rel > level){
            //     return i * size;
            // }
        }

        return res;
    }

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

public:
    T2SIN_FORM      t2sin;
    PREAMBLE_FORM   preamble;
    OFDM_FORM       message;

    complex_vector tx_frame_buf;
    complex16_vector tx_frame_int16_buf;

    complex_vector rx_frame_buf;
    complex16_vector rx_frame_int16_buf;

    int usefull_size;
    int output_size;

    bit_vector bit_preambple;

    FRAME_FORM(const std::string& CONFIGNAME);
    void write(bit_vector& input);
    
    bit_vector read(void* transmitted_data);

    complex_vector get();
    complex16_vector get_int16();

    void form_int16_to_double(){
        int len = rx_frame_buf.size();

        std::transform( rx_frame_int16_buf.begin(), rx_frame_int16_buf.end(),
                        rx_frame_buf.begin(),[](const std::complex<int16_t>& c){
                   return std::complex<double>(c.real(), c.imag());
               });
        
    }
    

};