#include <stdlib.h>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>
#include "modulation.hpp"
#include "parcer.hpp"
#include <fftw3.h>

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <optional>


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
    

    FFT_FORM(int fft_size,int num_data_subc,int num_pilot_subc, int num_symb)
        :   fft_size(fft_size),
            num_data_subc(num_data_subc),
            num_pilot_subc(num_pilot_subc),
            num_symb(num_symb),
            segment_step(num_data_subc/num_pilot_subc + 1),
            segment_size(segment_step-1),
            segment_byte_size(segment_size*sizeof(complex_double)),
            FFT_buf(num_symb*fft_size, complex_double(0.0, 0.0)),
            segment(num_pilot_subc*num_symb, nullptr),
            pilot(num_pilot_subc*num_symb, nullptr)
    {
        int num_pilot_subc_2 = num_pilot_subc/2;
        if (FFT_buf.size()){
            for(int i = 0, pilot_iter = 0, data_iter = 0; i < num_symb; i++, pilot_iter+=num_pilot_subc, data_iter+=fft_size){    
                int j = 0;
                for (int pos = 1+segment_size; j < num_pilot_subc_2; j++, pos+=segment_step){
                    pilot[pilot_iter+j] = FFT_buf.data()+data_iter+pos;
                    segment[pilot_iter+j] = FFT_buf.data()+data_iter+pos-segment_size;
                }
                for (int pos = fft_size-segment_step*num_pilot_subc_2; j < num_pilot_subc; j++, pos+=segment_step){
                    pilot[pilot_iter+j] = FFT_buf.data()+data_iter+pos;
                    segment[pilot_iter+j] = FFT_buf.data()+data_iter+pos+1;
                }
            }
        }
    }

    void write(complex_vector& input){
        for(auto &i : pilot)
            *i = REAL_ONE;

        auto input_ptr = input.data();
        for(int i = 0; i < pilot.size(); i++, input_ptr+=segment_size){
            memcpy(segment[i], input_ptr, segment_byte_size);

        }

            // Создаём план с пакетной обработкой
        fftw_plan plan = fftw_plan_many_dft(
            1,                 // rank (1D FFT)
            &fft_size,         // размер FFT
            num_symb,          // сколько батчей (howmany)
            reinterpret_cast<fftw_complex*>(FFT_buf.data()),         // вход
            nullptr,           // stride info (nullptr = обычный)
            1, fft_size,       // istride, idist (1 шаг внутри, расстояние = fft_size)
            reinterpret_cast<fftw_complex*>(FFT_buf.data()),        // выход
            nullptr,           // stride info
            1, fft_size,       // ostride, odist
            FFTW_BACKWARD,      // прямое преобразование
            FFTW_ESTIMATE      // стратегия
        );

        fftw_execute(plan);

        for (auto &x : FFT_buf) {
            x /= fft_size;
        }
    }
};


class T2SIN_FORM{

private:
    ConfigMap& config;
    
public:
    complex_double* buf = nullptr;
    int size;
    int f1;
    int f2;
    
    T2SIN_FORM(ConfigMap& config):
        config(config),
        size(config["T2sin_size"]),
        f1(config["T2_sin_f1"]),
        f2(config["T2_sin_f2"])
    {}

    void set(complex_double* buf_ptr){
        buf = buf_ptr;


        if (size){
            buf[f1] = REAL_ONE;
            buf[f2] = REAL_ONE;
        }
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



    OFDM_FORM(ConfigMap& config, bool data = true)
        : config(config),
          data(data),
          fft_size(config["fft_size"]),
          num_data_subc(config["num_data_subc"]),
          num_pilot_subc(config["num_pilot_subc"]),
          cp_size(config["cp_size"]),
          num_symb((data)?config["num_symb"]:config["num_pr_symb"]),
          pr_sin_len(config["pr_sin_len"]),
          pr_seed(config["pr_seed"]),
          modType(static_cast<mod_type>(config["modType"])),
          size((fft_size+cp_size)*num_symb),
          usefull_size(num_data_subc*num_symb),
          output(num_symb, nullptr),
          fft_task(fft_size, num_data_subc, num_pilot_subc, num_symb),
          Mod(modType)
    {}

    void set(complex_double* buf_ptr){
        for(int i = 0; i < num_symb; i++){
            output[i] = buf_ptr + (cp_size+fft_size)*i;
        }
    }

    void write(bit_vector& input){

        auto const_input = Mod.mod(input);
        
        fft_task.write(const_input);

        int byte_fft_size = fft_size*sizeof(complex_double);

        for(int i = 0, j = 0; i < num_symb; i++, j+=fft_size)
            memcpy(output[i]+cp_size, fft_task.FFT_buf.data()+j, byte_fft_size);

        int byte_cp_size = cp_size*sizeof(complex_double);

        for(int i = 0; i < num_symb; i++)
            memcpy(output[i], output[i]+fft_size, byte_cp_size);
    }
};


class FRAME_FORM{

private:
    ConfigMap config;

    T2SIN_FORM  t2sin;
    OFDM_FORM   preamble;
    OFDM_FORM   message;
    
    complex_vector frame_buf;

public:

    int usefull_size;

    FRAME_FORM(const std::string& CONFIGNAME)
        :   config(parse_config(CONFIGNAME)),
            t2sin(config),
            preamble(config, false),
            message(config),
            frame_buf(t2sin.size+preamble.size+message.size, complex_double(0.0, 0.0)),
            usefull_size(message.usefull_size*message.modType/8)
    {

        t2sin.set(frame_buf.data());
        preamble.set(frame_buf.data()+t2sin.size);
        message.set(frame_buf.data()+t2sin.size+preamble.size);
    }


    void write(bit_vector& input){
        message.write(input);
    }

    complex_vector get(){
        return frame_buf;
    }
    

};