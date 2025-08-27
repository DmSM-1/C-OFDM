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

class OFDM {
public:
    size_t total_subc;
    size_t free_subc;
    size_t pilots;
    size_t cp_len;
    size_t total_len;

    size_t pr_symp;
    size_t pr_sin_len;

    int pr_seed;

    size_t T2sin_len;

    int f1;
    int f2;

    mod_type modType;

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
            else if (key == "pr_symp") pr_symp = std::stoul(value);
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
    
    }
};