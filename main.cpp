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
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <iterator>
#include "io.hpp"
#include <Python.h>


int main(){
    
    FRAME_FORM tx_frame("config/config.txt");
    FRAME_FORM rx_frame("config/config.txt");

    // SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");
    // SDR rx_sdr(1, tx_frame.output_size, "config/config.txt");

    bit_vector origin_mes(tx_frame.usefull_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
 
    for (size_t i = 0; i < origin_mes.size(); i++) {
        origin_mes[i] = dis(gen);
    }
    
    tx_frame.write(origin_mes);
    
    auto mod_data   = tx_frame.get();
    auto tx_data    = tx_frame.get_int16();
  
    std::copy(tx_frame.int16_buf.begin(), tx_frame.int16_buf.end(), rx_frame.from_sdr_int16_buf.begin());    
    
    rx_frame.form_int16_to_double();
    


    auto t2_sin_corr = rx_frame.t2sin.corr(rx_frame.from_sdr_buf);

    write_complex_to_file("data/data.bin", rx_frame.from_sdr_buf);
    write_double_to_file("data/t2_sin_corr.bin", t2_sin_corr);


    Py_Initialize();
    PyRun_SimpleString(
        "import matplotlib.pyplot as plt\n"
        "import numpy as np\n"

        "arr = np.fromfile('data/data.bin', dtype = np.float64)\n"
        "arr = arr[::2] + 1j * arr[1::2]\n"
        "arr /= np.max(np.abs(arr))\n"
        "plt.plot(np.abs(arr))\n"

        "corr = np.fromfile('data/t2_sin_corr.bin', dtype = np.float64)\n"
        "corr /= np.max(corr)\n"
        "plt.plot(np.arange(0, arr.size, arr.size/corr.size), corr)\n"
        "plt.show()\n");
    Py_Finalize();

    // write_complex_to_file("data/tx_data.bin", tx_data.begin(), tx_data.end());
    // read_complex_from_file("data/tx_data.bin", rx_frame.rx_frame_int16_buf.begin());

    // tx_sdr.send(tx_data);
    // rx_sdr.recv(rx_frame.rx_frame_int16_buf);


    // rx_frame.form_int16_to_double();
    // auto symb_begin = rx_frame.t2sin.find_next_symb_with_t2sin(rx_frame.rx_frame_buf, 0);

    // rx_frame.preamble.find_cor_with_preamble(rx_frame.rx_frame_buf, symb_begin);
    
    // auto preamble_begin = rx_frame.preamble.find_start_symb_with_preamble(rx_frame.rx_frame_buf, symb_begin);
    
    // auto frame_begin = symb_begin+preamble_begin-rx_frame.t2sin.size;
    
    // memcpy(rx_frame.tx_frame_buf.data(), rx_frame.rx_frame_buf.data()+frame_begin, sizeof(complex_double)*(rx_frame.output_size));


    
    return 0;
}


