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


    int find_next_symb_with_t2sin(complex_vector& signal, int start_index){
        
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
        // std::cout<<"step "<<std::arg(step)<<'\n';
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

    double pilot_freq_shift() {
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

    for (int i = 0; i < size; i++) {
        amplitude[i] = std::abs(spec[i]);
    }

    double rel_bw = double(fft_size)/(fft_size+cp_size);
    double rel_pilot_w = rel_bw/num_pilot_subc;
    double rel_start = (1.0 - rel_bw - rel_pilot_w)/2.0;

    int pilot_w = int(size*rel_pilot_w);
    int start = int(size*rel_start);

    std::vector<double> amp_extended = amplitude;
    int amp_size = size;

    // Расширение спектра, если нужно
    if (start < 0) {
        start = 0;
        std::vector<double> extended(size*3, 0.0);
        std::copy(amplitude.begin(), amplitude.end(), extended.begin()+size);
        amp_extended.swap(extended);
        amp_size = static_cast<int>(amp_extended.size());
    }

    std::vector<int> top_indexes(num_pilot_subc+1);
    for (int i = 0; i <= num_pilot_subc; i++) {
        int seg_start = start + i*pilot_w;
        if (seg_start >= amp_size) break; // не выходим за границы
        int seg_end = std::min(seg_start + pilot_w, amp_size);

        auto max_it = std::max_element(amp_extended.begin() + seg_start, amp_extended.begin() + seg_end);
        top_indexes[i] = static_cast<int>(std::distance(amp_extended.begin(), max_it));
    }

    // Удаляем центральный пилот
    if (!top_indexes.empty() && num_pilot_subc/2 < top_indexes.size()) {
        top_indexes.erase(top_indexes.begin() + num_pilot_subc/2);
    }

    double sum = std::accumulate(top_indexes.begin(), top_indexes.end(), 0.0);
    double mean = sum / top_indexes.size();
    double shift = mean - size/2.0;

    return shift/size;
}

    void freq_shift(double& shift){
        complex_double step = std::exp(complex_double(0, -2*M_PIf64*shift));
        complex_double phase = complex_double(1, 0);

        for(int i = 0; i < size; i++){
            output[0][i] *= phase;
            phase *= step;
        }
    }

};


class PREAMBLE_FORM : public OFDM_FORM{

public:
    double level;
    bit_vector preamble;
    complex_vector mod_preamble;
    complex_vector conjected_sinh_part;
    std::vector<double> cor;

    PREAMBLE_FORM(ConfigMap& config);
    void set(complex_double* buf_ptr);

    void find_cor_with_preamble(complex_vector& input, int start);
    int find_start_symb_with_preamble(complex_vector& input, int start);

};


class FRAME_FORM{

private:
    ConfigMap config;

public:
    T2SIN_FORM      t2sin;
    PREAMBLE_FORM   preamble;
    OFDM_FORM       message;
    OFDM_FORM       message_with_preamble;

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