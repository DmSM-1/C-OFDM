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
    FRAME_FORM rx_frame("config/config.txt");

    MAC mac(1, 0, tx_frame.usefull_size);
    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");

    bit_vector origin_mes(mac.payload);
    FILE* file = fopen("Rammstein-Radio.wav", "rb");
    
    while (fread(origin_mes.data(), 1, origin_mes.size(), file)){    

        auto tx_mac_frame = mac.write(origin_mes, 0);
        tx_frame.message.Mod.scrembler(tx_mac_frame.data(), tx_mac_frame.size());
        rx_frame.write(tx_mac_frame);
        auto mod_data = rx_frame.get();
        auto tx_data = rx_frame.get_int16();  
        
        tx_sdr.send(tx_data);
    }

    fclose(file);
    
    return 0;
}


