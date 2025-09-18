#include "modulation.hpp"


complex_double psk(uint8_t input, double angle, int deg){
    double step = M_PI*2/(double)deg;
    complex_double j(0.0, 1.0);

    return std::exp(j*(step*complex_double(input)+angle));
}


complex_double qam(uint8_t input, int deg){
    if ((deg%2) || deg > 8)
        return complex_double(0.0, 0.0);

    complex_double j(0.0, 1.0);
    uint8_t num = 1u<<(deg/2);

    return complex_double(2.0/(num-1)*double(input%num)-1.0, 2.0/(num-1)*double(input>>(deg/2))-1.0);
}


Modulation::Modulation(mod_type mod, int len_buf)
:   str_size(1u<<(mod/2)),
    modulation(mod), 
    constell(1u<<mod, 0)
{
    mod_index = modulation;
    step = 2.0/(str_size-1);
    str_size_1 = 1.0/step;

    if (modulation == bpsk){
        for (size_t i = 0; i < constell.size(); i++)
            constell[i] = psk(uint8_t(i), M_PI_4*5, 2); 
    }else{
        for (size_t i = 0; i < constell.size(); i++)
            constell[i] = qam(uint8_t(i), mod_index); 
    }
    
}


complex_vector Modulation::mod(std::vector<uint8_t>& bin_input){

    auto input = bit_stream_converter(mod_index, 8, bin_input);
    size_t len = input.size();

    complex_vector output(len, complex_double(0,0));

    for (size_t i = 0; i < len; i++)
        output[i] = constell[input[i]];

    return output;
}


std::vector<uint8_t> Modulation::demod(complex_vector& input){
    size_t len = input.size();

    if (len!=demod_buffer.size())
        demod_buffer.resize(len);

    switch (modulation)
    {
    case bpsk:
        for(size_t i = 0; i < len; ++i)
            demod_buffer[i] = uint8_t(input[i].real() + input[i].imag() > 0);
        
        
    break;
    
    default:{
        for (auto &i : input){
            i = complex_double(
                std::clamp(i.real(), -1.0, 1.0),
                std::clamp(i.imag(), -1.0, 1.0)
            );
        }

        for(size_t i = 0; i < len; ++i)
            demod_buffer[i] = uint8_t((input[i].real()+1.0)*str_size_1+0.5)|
                            uint8_t(((input[i].imag()+1.0)*str_size_1+0.5))*str_size;
             
    }
        break;
    }

    auto bin_output = bit_stream_converter(8, mod_index, demod_buffer);

    return bin_output;
}


std::vector<uint8_t> Modulation::bit_stream_converter(size_t output_block_size, size_t input_block_size, std::vector<uint8_t>& input){

    size_t len = input.size();
    size_t output_len = (len*input_block_size)/output_block_size + ((len*input_block_size)%output_block_size>0);
    
    uint8_t mask;
    std::vector<uint8_t> output(output_len, 0);
    auto output_iter = output.begin();
    auto input_iter = input.begin();
    
    mask = 1<<(input_block_size-1);

    for (size_t i = 0, j = 0; i < input_block_size*len; i++){
        if (j==output_block_size){
            j=0;
            output_iter++;
        }
        j++;
        *output_iter <<= 1;
        
        if ((mask&(*input_iter))>0){
            (*output_iter)++;
        }

        mask >>= 1;
        if (mask==0){
            mask = 1<<(input_block_size-1);
            input_iter++;
        }
    }

    if ((len*input_block_size)%output_block_size > 0)
        output[output_len-1] <<= output_block_size - (len*input_block_size)%output_block_size;

    return output;
}
