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
    f2(config["T2_sin_f2"]),
    smooth(config["smooth"]),
    level((double)config["T2_sin_level"]/1000),
    detect_mask(size, 0.0),
    detect_buf(size, complex_double(0, 0)),
    detect_plan(fftw_plan_many_dft(
        1, &size, 1,
        reinterpret_cast<fftw_complex*>(detect_buf.data()),nullptr, 1, size,
        reinterpret_cast<fftw_complex*>(detect_buf.data()),nullptr, 1, size,
        FFTW_FORWARD, FFTW_MEASURE)),

    mean_freq((f1+f2)/2),
    min_f1(std::max(0, f1-smooth)),
    max_f1(std::max(mean_freq, f1+smooth)),
    min_f2(std::max(mean_freq, f2-smooth)),
    max_f2(std::max(size, f2+smooth))
{
    int a1 = std::max(0, f1 - (int)config["smooth"]);
    int b1 = std::min(size - 1, f1 + (int)config["smooth"]);
    int a2 = std::max(0, f2 - (int)config["smooth"]);
    int b2 = std::min(size - 1, f2 + (int)config["smooth"]);

    int sum = (b1 - a1 + 1) + (b2 - a2 + 1);

    double val = 1.0 / sum;

    for (int i = a1; i <= b1; i++) {
        detect_mask[i] += val;
    }

    for (int i = a2; i <= b2; i++) {
        detect_mask[i] += val;
    }


}


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


OFDM_FORM::OFDM_FORM(ConfigMap& config, bool data, bool with_preamble)
        : config(config),
          data(data),
          fft_size(config["fft_size"]),
          num_data_subc(config["num_data_subc"]),
          num_pilot_subc(config["num_pilot_subc"]),
          cp_size(config["cp_size"]),
          num_symb((with_preamble)?config["num_symb"]+config["num_pr_symb"]:(data)?config["num_symb"]:config["num_pr_symb"]),
          pr_sin_len(config["pr_sin_len"]),
          pr_seed(config["pr_seed"]),
          modType(static_cast<mod_type>(config["modType"])),
          ofdm_len(fft_size+cp_size),
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
        message_with_preamble(config, true, true),
        tx_frame_buf(t2sin.size+preamble.size+message.size, complex_double(0.0, 0.0)),
        tx_frame_int16_buf(tx_frame_buf.size()),
        rx_frame_buf(tx_frame_buf.size()*config["rx_buf_size"], complex_double(0.0, 0.0)),
        rx_frame_int16_buf(rx_frame_buf.size()),
        usefull_size(message.usefull_size*message.modType/8),
        output_size(tx_frame_buf.size()),
        bit_preambple(usefull_size, 0)
{

    t2sin.set(tx_frame_buf.data());
    preamble.set(tx_frame_buf.data()+t2sin.size);
    message.set(tx_frame_buf.data()+t2sin.size+preamble.size);
    message_with_preamble.set(tx_frame_buf.data()+t2sin.size);
}


void FRAME_FORM::write(bit_vector& input){
    message.write(input);
}

bit_vector FRAME_FORM::read(void* transmitted_data){
    memcpy(tx_frame_buf.data(), transmitted_data, sizeof(complex_double)*tx_frame_buf.size());
    return message.read();
}

complex_vector FRAME_FORM::get(){
    return tx_frame_buf;
}


complex16_vector FRAME_FORM::get_int16(){
    int len = tx_frame_buf.size();
    for (int i = 0; i < len; i++){
        tx_frame_buf[i] *= config["mult"];
        tx_frame_int16_buf[i] = std::complex<int16_t>(tx_frame_buf[i]);
        // frame_int16_buf[i] *= 16;
    }
    return tx_frame_int16_buf;
}


PREAMBLE_FORM::PREAMBLE_FORM(ConfigMap& config)
:   OFDM_FORM(config, false),
    level((double)config["pr_level"]),
    preamble(usefull_size*modType/8, 0),
    mod_preamble(size, complex_double(0,0)),
    conjected_sinh_part(pr_sin_len, complex_double(0,0)),
    cor((int)config["T2sin_size"]*2 + pr_sin_len, 0.0)
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

    for (int i = 0; i < pr_sin_len; i++){
        conjected_sinh_part[i] = std::conj(mod_preamble[i]);
    }
}


void PREAMBLE_FORM::find_cor_with_preamble(complex_vector& input, int start){

    std::fill(cor.begin(), cor.end(), 0.0);
    double norm = 0;
    double re, im;

    auto input_ptr = input.data()+start;

    for (int i = 0; i < pr_sin_len; i++, input_ptr++){
        re = input_ptr->real();
        im = input_ptr->imag();
        norm += re*re+im*im;
    }

    input_ptr = input.data()+start;

    int cycles = cor.size();
    complex_double energy;

    for (int i = 0; i < cycles; i++, input_ptr++){
        energy = complex_double(0,0);

        if (norm > 1.0){
            for (int j = 0; j < pr_sin_len; j++)
                energy += input_ptr[j]*conjected_sinh_part[j];
            
            cor[i] = std::abs(energy);
            cor[i] /= std::sqrt(norm);
        }

        re = input_ptr[pr_sin_len].real();
        im = input_ptr[pr_sin_len].imag();
        norm += re*re+im*im;

        re = input_ptr[0].real();
        im = input_ptr[0].imag();
        norm -= re*re+im*im;
    }
}


int PREAMBLE_FORM::find_start_symb_with_preamble(complex_vector& input, int start){

    // std::fill(cor.begin(), cor.end(), 0.0);
    double norm = 0;
    double re, im;

    auto input_ptr = input.data()+start;

    for (int i = 0; i < pr_sin_len; i++, input_ptr++){
        re = input_ptr->real();
        im = input_ptr->imag();
        norm += re*re+im*im;
    }

    input_ptr = input.data()+start;

    int cycles = cor.size();
    complex_double energy;

    for (int i = 0; i < cycles; i++, input_ptr++){
        energy = complex_double(0,0);

        if (norm > 1.0){
            for (int j = 0; j < pr_sin_len; j++)
                energy += input_ptr[j]*conjected_sinh_part[j];
            
            if (std::abs(energy)/std::sqrt(norm) > level)
                return i;
        }

        re = input_ptr[pr_sin_len].real();
        im = input_ptr[pr_sin_len].imag();
        norm += re*re+im*im;

        re = input_ptr[0].real();
        im = input_ptr[0].imag();
        norm -= re*re+im*im;
    }

    return 0;
}