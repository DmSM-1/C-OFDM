#include <stdlib.h>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>
#include "modulation.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <optional>

class OFDM {
public:
    size_t total_subc;
    size_t free_subc;
    size_t pilots;
    size_t cp_len;
    size_t total_len;
    size_t symb;

    size_t pr_symb;
    size_t pr_sin_len;

    int pr_seed;

    size_t T2sin_len;

    size_t full_len;
    size_t total_symb;

    int f1;
    int f2;

    size_t seg_size;
    size_t byte_seg_size;
    size_t usefull_seg_num;
    size_t usefull_byte_size;
    size_t usefull_size;

    mod_type modType;

    std::optional<Modulation> Mod;
    complex_vector* demod_buf = nullptr;
    complex_double* ofdm_buf = nullptr;
    complex_double** pilot_pos = nullptr;
    complex_double** segments = nullptr;
    complex_double** data_segments = nullptr;

    OFDM(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) throw std::runtime_error("Cannot open config file");

        std::string line;
        while (std::getline(file, line)) {
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch){ return !std::isspace(ch); }));
            line.erase(std::find_if(line.rbegin(), line.rend(), [](int ch){ return !std::isspace(ch); }).base(), line.end());

            if (line.empty() || line[0]=='#') continue;

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos+1);

            key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
            value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

            if (key == "total_subc") total_subc = std::stoul(value);
            else if (key == "free_subc") free_subc = std::stoul(value);
            else if (key == "pilots") pilots = std::stoul(value);
            else if (key == "cp_len") cp_len = std::stoul(value);
            else if (key == "symb") symb = std::stoul(value);
            else if (key == "pr_symb") pr_symb = std::stoul(value);
            else if (key == "pr_sin_len") pr_sin_len = std::stoul(value);
            else if (key == "pr_seed") pr_seed = std::stoi(value);
            else if (key == "T2sin_len") T2sin_len = std::stoul(value);
            else if (key == "f1") f1 = std::stoi(value);
            else if (key == "f2") f2 = std::stoi(value);
            else if (key == "modType") {
                if (value == "BPSK") modType = bpsk;
                else if (value == "QPSK") modType = qam4;
                else if (value == "16QAM") modType = qam16;
                else if (value == "64QAM") modType = qam64;
                else if (value == "256QAM") modType = qam256;
            }
        }
        full_len = T2sin_len + (total_subc+cp_len)*(pr_symb+symb);
        total_symb = pr_symb+symb;

        Mod.emplace(modType);

        
        int pilot_step = (total_subc-2-free_subc)/pilots;
        seg_size = pilot_step-1;
        byte_seg_size = sizeof(complex_double)*seg_size;
        usefull_size = seg_size*pilots*symb;
        usefull_byte_size = (usefull_size*(Mod->mod_index))/8;
        usefull_seg_num = pilots*symb;
        
        ofdm_buf    = new complex_double[full_len]();
        demod_buf   = new complex_vector(usefull_seg_num*seg_size, 0);
        pilot_pos   = new complex_double*[pilots*(total_symb)]();
        segments    = new complex_double*[pilots*(total_symb)]();

        data_segments = segments+pilots*pr_symb;
        
        for(size_t i = 0, pos = 0; i < total_symb; i++){
            pos = T2sin_len+i*(cp_len+total_subc)+cp_len+1;
            size_t j = 0;
            for(; j < pilots/2; j++){
                segments[i*pilots+j]  = ofdm_buf+pos;
                pilot_pos[i*pilots+j] = ofdm_buf+pos+pilot_step-1;
                pos += pilot_step;
            }
            pos = T2sin_len+i*(cp_len+total_subc)+cp_len+total_subc-(pilots/2)*pilot_step;
            for(; j < pilots; j++){
                segments[i*pilots+j]    = ofdm_buf+pos+1;
                pilot_pos[i*pilots+j]   = ofdm_buf+pos;
                pos += pilot_step;
            }
        }


        for(size_t i = 0; i < pilots*total_symb; i++){
            *pilot_pos[i] = complex_double(1.0, 0.0);
        }

        if (T2sin_len){
            ofdm_buf[f1] = complex_double(1.0, 0.0);
            ofdm_buf[f2] = complex_double(1.0, 0.0);
        }
    }


    ~OFDM(){
        delete[] ofdm_buf;
        delete[] pilot_pos;
        delete[] segments;
        delete demod_buf;
    }


    void mod(std::vector<uint8_t>& input){
        input.resize(usefull_byte_size);
        auto mod_input = Mod->mod(input);
        auto input_pointer = mod_input.data();
        for(size_t i = 0; i < usefull_seg_num; i++){
            memcpy(data_segments[i], input_pointer, byte_seg_size);
            input_pointer+=seg_size;
        }
        
    }
    
    std::vector<uint8_t> demod(){
        auto buf_pointer = demod_buf->data();
        for(size_t i = 0; i < usefull_seg_num; i++){
            memcpy(buf_pointer, data_segments[i], byte_seg_size);
            buf_pointer+=seg_size;
        }
        return Mod->demod(*demod_buf);
    }

};