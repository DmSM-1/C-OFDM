#pragma once

#include <stdlib.h>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>
#include "modulation.hpp"
#include "../config/parser.hpp"
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
    double pilot_ampl;

    FFT_FORM(int fft_size,int num_data_subc,int num_pilot_subc, int num_symb, double pilot_ampl = 1.0);
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
    int smooth;
    
    double level;
    
    std::vector<double> detect_mask;
    complex_vector detect_buf;
    
    fftw_plan detect_plan;
    
    int mean_freq;
    int min_f1;
    int max_f1;
    
    int min_f2;
    int max_f2;
    
    int real_f1 = 0;
    int real_f2 = 0;
    int real_f1_ampl = 0;
    int real_f2_ampl = 0;
    double freq_shift = 0;

    complex_double* buf = nullptr;


    T2SIN_FORM(ConfigMap& config);
    void set(complex_double* buf_ptr);


        std::vector<double> corr(complex_vector& signal){
        
        int cycles      = signal.size()/size;

        std::vector<double> corr(cycles, 0.0);

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

            if (rel > level){

                corr[i] = rel;
            }
        }

        return corr;
    }


    int find_t2sin(complex_vector& signal, int start_index){
        
        int cycles      = signal.size()/size;

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

            if (rel > level){

                return i*size;
            }
        }

        return -1;
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

    int ofdm_len;
    int size;
    int usefull_size;

    std::vector<complex_double*> output;

    FFT_FORM fft_task;

    Modulation Mod;

    int byte_fft_size;
    int pilot_ampl;

    OFDM_FORM(ConfigMap& config, bool data = true, bool with_preamble = false);
    virtual void set(complex_double* buf_ptr);
    void write(bit_vector& input);
    bit_vector read();

    void cp_freq_sinh(){
        complex_double phase;
        complex_double step;
        complex_double shift;
        for (int i=0; i<size; i+= ofdm_len){
            phase = complex_double(0);
            shift = complex_double(1,0);
            for (int j = 0; j < cp_size; j++){
                phase += std::conj(output[0][i+j])*output[0][i+j+fft_size];
            }
            step = std::exp(-complex_double(0, 1)*(std::arg(phase)/fft_size));
            for (int j = i; j < size; j++){
                output[0][j] *= shift;
                shift *= step;
            }
        }
    }

    void pr_phase_sinh(complex_double* pr, int pr_size){
        complex_double phase = complex_double(0);
        for(int i = 0; i < pr_size; i++){
            phase += std::conj(pr[i])*output[0][i];
        }
        phase = std::exp(-complex_double(0, 1)*(std::arg(phase)));
        for(int i = 0; i < size; i++){
            output[0][i] *= phase;
        }
    }

    complex_vector fft(){

        for(int i = 0, j = 0; i < num_symb; i++, j+=fft_size)
            memcpy(fft_task.FFT_buf.data()+j, output[i]+cp_size, byte_fft_size);

        return fft_task.read();   
    }


    double pilot_freq_sinh(){
        complex_vector spec(size, complex_double(0.0));
        std::vector<double> amplitude(size, 0.0);

        fftw_plan plan = fftw_plan_dft_1d(
            size,
            reinterpret_cast<fftw_complex*>(output[0]),
            reinterpret_cast<fftw_complex*>(spec.data()),
            FFTW_FORWARD,
            FFTW_ESTIMATE
        );

        fftw_execute(plan);
        fftw_destroy_plan(plan);

        complex_vector shifted(size);
        int half = size / 2;
        for (int i = 0; i < half; i++) {
            shifted[i] = spec[i + half];
            shifted[i + half] = spec[i];
        }
        
        for (int i = 0; i < size; i++) {
            amplitude[i] = std::abs(shifted[i]);
        }

        double rel_bw = double(num_data_subc+num_pilot_subc)/(fft_size);
        double rel_pilot_w = rel_bw/num_pilot_subc;
        int pilot_w = int(size*rel_pilot_w);
        std::vector<int> borders(num_pilot_subc+2, 0);

        for (int i = 0, j = int((1.0 - rel_bw - rel_pilot_w)/2.0*size); i < num_pilot_subc+2; i++){
            borders[i] = j;
            j += pilot_w;
        }

        borders[0] = std::max(0, borders[0]);
        borders[num_data_subc+1] = std::min(size, borders[num_data_subc+1]);

        double shift = 0;
        for (int i = 0; i < num_pilot_subc+1; i++){
            if (i == num_pilot_subc/2)
                continue;
            
            auto it = std::max_element(amplitude.begin()+borders[i], amplitude.begin()+borders[i+1]); 
            shift += std::distance(amplitude.begin(), it);
        }
        shift /= num_pilot_subc;
        shift -= size/2;
        shift /= size;

        return shift;
    }


    void freq_shift(double& shift){
        complex_double step = std::exp(complex_double(0, -2*M_PIf64*shift));
        complex_double phase = complex_double(1, 0);

        for(int i = 0; i < size; i++){
            output[0][i] *= phase;
            phase *= step;
        }
    }

    // std::vector<double> phases




};


class PREAMBLE_FORM : public OFDM_FORM{

public:
    double level;
    bit_vector preamble;
    complex_vector mod_preamble;
    complex_vector ofdm_preamble;
    complex_vector conjected_sinh_part;
    std::vector<double> cor;
    complex_vector chan_est;

    PREAMBLE_FORM(ConfigMap& config);
    void set(complex_double* buf_ptr);

    void find_corr(complex_vector& input, int start);
    int find_preamble(complex_vector& input, int start);

    complex_vector chan_char(){
        auto pr = fft();
        std::fill(chan_est.begin(), chan_est.end(), complex_double(0.0, 0.0));
        for (int i = 0; i < num_data_subc*num_symb; i++){
            chan_est[i%num_data_subc] += pr[i]/mod_preamble[i];
        }
        for (int i = 0; i < num_data_subc; i++){
            chan_est[i] /= complex_double(num_symb, 0);
        }
        return chan_est;
    }

        complex_vector chan_char_lq(){
        auto pr = fft();

        double mean_x   = 0.0;
        double mean_y   = 0.0;
        double mean_xy  = 0.0;
        double mean_x2  = 0.0;
        double a        = 0.0;
        double b        = 0.0;

        std::fill(chan_est.begin(), chan_est.end(), complex_double(0.0, 0.0));
        
        std::vector<double> phase(chan_est.size()/2, 0.0);

        for (int i = 0; i < phase.size(); i++){
            phase[i] = std::arg(pr[i]/mod_preamble[i]);
        }

        for (int i = 1; i < phase.size(); i++) {
            double phase_diff = phase[i] - phase[i-1];
            if (phase_diff > M_PI) {
                phase[i] -= 2 * M_PI;
            } else if (phase_diff < -M_PI) {
                phase[i] += 2 * M_PI;
            }
        }

        for(int i = 0; i < phase.size(); i++){
            mean_xy += phase[i]*i;
            mean_x2 += i*i;
            mean_x += i;
            mean_y += phase[i];
        }
        b = (mean_xy - mean_x*mean_y)/(mean_x2 - mean_x*mean_x);
        a = mean_y - b*mean_x;
        
        for(int i = 0; i < chan_est.size()/2; i++){
            chan_est[i] = std::exp(complex_double(0, b*i+a));
        }
        for(int i = chan_est.size()/2; i < chan_est.size(); i++){
            chan_est[i] = std::exp(complex_double(0, -b*chan_est.size()/2+(i-chan_est.size()/2)*b+a));
        }


        return chan_est;
    }
    



};


class FRAME_FORM{

private:
    ConfigMap config;

public:
    T2SIN_FORM      t2sin;
    PREAMBLE_FORM   preamble;
    OFDM_FORM       message;
    OFDM_FORM       message_with_preamble;

    complex_vector buf;
    complex16_vector int16_buf;

    complex_vector from_sdr_buf;
    complex16_vector from_sdr_int16_buf;

    int usefull_size;
    int output_size;

    bit_vector bit_preambple;

    FRAME_FORM(const std::string& CONFIGNAME);
    void write(bit_vector& input);
    
    bit_vector read(void* transmitted_data);

    complex_vector get();
    complex16_vector get_int16();

    void form_int16_to_double(){

        std::transform( from_sdr_int16_buf.begin(), from_sdr_int16_buf.end(),
                        from_sdr_buf.begin(),[](const std::complex<int16_t>& c){
                   return std::complex<double>(c.real(), c.imag());
               });
        
    }
    

};