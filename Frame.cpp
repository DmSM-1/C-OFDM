#include "Frame.hpp"


FFT_FORM::FFT_FORM(int fft_size,int num_data_subc,int num_pilot_subc, int num_symb)
    :   fft_size(fft_size),
        num_data_subc(num_data_subc),
        num_pilot_subc(num_pilot_subc),
        num_symb(num_symb),
        segment_step(num_data_subc/num_pilot_subc + 1),
        segment_size(segment_step-1),
        segment_byte_size(segment_size*sizeof(complex_double)),
        FFT_buf(num_symb*fft_size, complex_double(0.0, 0.0)),
        segment(num_pilot_subc*num_symb, nullptr),
        pilot(num_pilot_subc*num_symb, nullptr),

        backward_plan(fftw_plan_many_dft(1,&fft_size,num_symb,
        reinterpret_cast<fftw_complex*>(FFT_buf.data()),nullptr, 1, fft_size,       
        reinterpret_cast<fftw_complex*>(FFT_buf.data()),nullptr, 1, fft_size,       
        FFTW_BACKWARD, FFTW_MEASURE)),

        forward_plan(fftw_plan_many_dft(1,&fft_size,num_symb,
        reinterpret_cast<fftw_complex*>(FFT_buf.data()),nullptr, 1, fft_size,       
        reinterpret_cast<fftw_complex*>(FFT_buf.data()),nullptr, 1, fft_size,       
        FFTW_FORWARD, FFTW_MEASURE)),

        restored_buf(num_data_subc*num_symb, 0),

        norm_factor(sqrt(static_cast<double>(fft_size)))
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



void FFT_FORM::write(complex_vector& input){
    for(auto &i : pilot)
        *i = REAL_ONE;

    auto input_ptr = input.data();
    for(size_t i = 0; i < pilot.size(); i++, input_ptr+=segment_size){
        memcpy(segment[i], input_ptr, segment_byte_size);   
    }

    fftw_execute(backward_plan);

    for (auto &x : FFT_buf) {
        x /= norm_factor;
    }
}


complex_vector& FFT_FORM::read(){
    fftw_execute(forward_plan);

    for (auto &x : FFT_buf) {
        x /= norm_factor;
    }

    auto input_ptr = restored_buf.data();
    for(size_t i = 0; i < pilot.size(); i++, input_ptr+=segment_size){
        memcpy(input_ptr, segment[i], segment_byte_size);   
    }

    return restored_buf;
}


T2SIN_FORM::T2SIN_FORM(ConfigMap& config):
    config(config),
    size(config["T2sin_size"]),
    f1(config["T2_sin_f1"]),
    f2(config["T2_sin_f2"])
{}


void T2SIN_FORM::set(complex_double* buf_ptr){
    buf = buf_ptr;

    if (size){
        buf[f1] = complex_double(0.5,0);
        buf[f2] = complex_double(0.5,0);
    }

    fftw_plan backward_plan = fftw_plan_many_dft(1,&size,1,
        reinterpret_cast<fftw_complex*>(buf),nullptr, 1, size,       
        reinterpret_cast<fftw_complex*>(buf),nullptr, 1, size,       
        FFTW_BACKWARD, FFTW_ESTIMATE);

    fftw_execute(backward_plan);

}


OFDM_FORM::OFDM_FORM(ConfigMap& config, bool data)
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
          Mod(modType),
          byte_fft_size(fft_size*sizeof(complex_double))
{}

void OFDM_FORM::set(complex_double* buf_ptr){
    for(int i = 0; i < num_symb; i++){
        output[i] = buf_ptr + (cp_size+fft_size)*i;
    }
}


void OFDM_FORM::write(bit_vector& input){

    auto const_input = Mod.mod(input);
    
    fft_task.write(const_input);

    for(int i = 0, j = 0; i < num_symb; i++, j+=fft_size)
        memcpy(output[i]+cp_size, fft_task.FFT_buf.data()+j, byte_fft_size);

    int byte_cp_size = cp_size*sizeof(complex_double);

    for(int i = 0; i < num_symb; i++)
        memcpy(output[i], output[i]+fft_size, byte_cp_size);
}


bit_vector OFDM_FORM::read(){

    for(int i = 0, j = 0; i < num_symb; i++, j+=fft_size)
        memcpy(fft_task.FFT_buf.data()+j, output[i]+cp_size, byte_fft_size);

    return Mod.demod(fft_task.read());
    
}


FRAME_FORM::FRAME_FORM(const std::string& CONFIGNAME)
    :   config(parse_config(CONFIGNAME)),
        t2sin(config),
        preamble(config),
        message(config),
        frame_buf(t2sin.size+preamble.size+message.size, complex_double(0.0, 0.0)),
        usefull_size(message.usefull_size*message.modType/8),
        bit_preambple(usefull_size, 0)
{

    t2sin.set(frame_buf.data());
    preamble.set(frame_buf.data()+t2sin.size);
    message.set(frame_buf.data()+t2sin.size+preamble.size);
    // std::cout<<t2sin.size<<' '<<preamble.size<<' '<<message.size<<'\n';
}


void FRAME_FORM::write(bit_vector& input){
    message.write(input);
}

bit_vector FRAME_FORM::read(void* transmitted_data){
    memcpy(frame_buf.data(), transmitted_data, sizeof(complex_double)*frame_buf.size());
    return message.read();
}

complex_vector FRAME_FORM::get(){
    return frame_buf;
}


PREAMBLE_FORM::PREAMBLE_FORM(ConfigMap& config)
:   OFDM_FORM(config, false),
    preamble(usefull_size*modType/8, 0),
    mod_preamble(size, complex_double(0,0))
{
    std::mt19937 rng(pr_seed);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto &i : preamble)
        i = dist(rng);
}


void PREAMBLE_FORM::set(complex_double* buf_ptr){
    for(int i = 0; i < num_symb; i++){
        output[i] = buf_ptr + (cp_size+fft_size)*i;
    }
    write(preamble);
    memcpy(mod_preamble.data(), output[0], mod_preamble.size()*sizeof(complex_double));
}

