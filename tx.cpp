#include <iostream>
#include <stdlib.h>
#include <cstring>
#include "OFDM/modulation.hpp"
#include <vector>
#include <chrono>
#include <fstream>
#include <random>
#include "OFDM/Frame.hpp"
#include "sdr/sdr.hpp"
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <iterator>
#include "io/io.hpp"
#include "mac/mac_frame.hpp"


int main(){
    
    usleep(200000);

    FRAME_FORM tx_frame("config/config.txt");
    MAC mac(1, 0, tx_frame.usefull_size);
    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");

    bit_vector origin_mes(mac.payload);
    int frames = tx_frame.config["frames"];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    FILE * file;

    char filename[64] = {0};
    
    for (int i = 0; i < frames; i++){

        for (auto &i : origin_mes)
            i = dis(gen);

        for (auto &i : origin_mes)
            i = dis(gen);

        auto tx_mac_frame = mac.write(origin_mes, 0);

        sprintf(filename, "frames/tx_mac_frame_%d", i);
        file = fopen(filename, "w");
        fprintf(file, (char*)tx_mac_frame.data(), tx_mac_frame.size());
        fclose(file);

        tx_frame.message.Mod.scrembler(tx_mac_frame.data(), tx_mac_frame.size());
        tx_frame.write(tx_mac_frame);
        auto mod_data = tx_frame.get();
        auto tx_data = tx_frame.get_int16();  
            
        tx_sdr.send(tx_data);
    }
    
    return 0;
}


