#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <mkl.h>
#include "modulation.hpp"
#include <vector>

std::vector<uint8_t> bit_stream_converter(size_t output_block_size, size_t input_block_size, std::vector<uint8_t> input){

    size_t len = input.size();
    size_t output_len = (len*input_block_size)/output_block_size + ((len*input_block_size)%output_block_size>0);
    
    uint8_t mask;
    std::vector<uint8_t> output(output_len, 0);
    auto output_iter = output.begin();
    auto input_iter = input.begin();
    
    mask = 1<<(input_block_size-1);

    for (int i = 0, j = 0; i < input_block_size*len; i++){
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

int main(){
    const char* SFILE_NAME = "text.txt";

    std::vector<uint8_t> buf(1024);
    FILE* SFILE = fopen(SFILE_NAME, "r");
    
    fread(buf.data(), 1, 1024, SFILE);

    std::vector<uint8_t> inter = bit_stream_converter(1, 8, buf);

    
    // std::vector<uint8_t> res = bit_stream_converter(8, 7, inter);
    
    // for (int i = 0; i < inter.size(); i++){
    //     std::cout<<int(inter[i])<<" ";
    // }
    // std::cout<<'\n';

    Modulation Mod(bpsk);
    auto mod_data = Mod.mod(inter);
    auto demod_data = Mod.demod(mod_data);

    std::vector<uint8_t> res = bit_stream_converter(8, 1, inter);

    for (int i = 0; i < res.size(); i++){
        std::cout<<res[i];
    }

    std::cout<<'\n';

    return 0;
}


