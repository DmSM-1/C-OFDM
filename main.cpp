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
#include "io/io.hpp"
#include <Python.h>


int main(){
    
    FRAME_FORM tx_frame("config/config.txt");
    FRAME_FORM rx_frame("config/config.txt");

    SDR tx_sdr(0, tx_frame.output_size, "config/config.txt");
    SDR rx_sdr(1, tx_frame.output_size, "config/config.txt");


    bit_vector origin_mes(tx_frame.usefull_size);
    FILE* file = fopen("WARANDPEACE.txt", "r");
    fread(origin_mes.data(), 1, origin_mes.size(), file);
    fclose(file);
    
    tx_frame.write(origin_mes);
    
    auto mod_data   = tx_frame.get();
    auto tx_data    = tx_frame.get_int16();
  
    std::copy(tx_frame.int16_buf.begin(), tx_frame.int16_buf.end(), rx_frame.from_sdr_int16_buf.begin());    

    tx_sdr.send(tx_data);
    rx_sdr.recv(rx_frame.from_sdr_int16_buf);
    
    rx_frame.form_int16_to_double();
    
    auto t2_sin_corr = rx_frame.t2sin.corr(rx_frame.from_sdr_buf);
    auto t2_sin_begin = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, 0);

    write_complex_to_file("data/data.bin", rx_frame.from_sdr_buf);
    write_complex_to_file("data/spec.bin", rx_frame.t2sin.detect_buf);
    write_double_to_file("data/t2_sin_corr.bin", t2_sin_corr);
    write_complex_to_file("data/row.bin", rx_frame.from_sdr_buf.begin()+t2_sin_begin, rx_frame.from_sdr_buf.begin()+t2_sin_begin+rx_frame.output_size);
    
    rx_frame.preamble.find_corr(rx_frame.from_sdr_buf, t2_sin_begin);
    auto pr_begin = rx_frame.preamble.find_preamble(rx_frame.from_sdr_buf, t2_sin_begin)+1;

    write_double_to_file("data/pr_corr.bin", rx_frame.preamble.cor);

    // std::cout<<"BEGIN: "<<t2_sin_begin<<" "<<pr_begin<<"\n";

    std::copy(
        rx_frame.from_sdr_buf.begin()+t2_sin_begin+pr_begin-rx_frame.t2sin.size, 
        rx_frame.from_sdr_buf.begin()+t2_sin_begin+pr_begin-rx_frame.t2sin.size+rx_frame.output_size, 
        rx_frame.buf.begin());
    
    auto freq_shift = rx_frame.message_with_preamble.pilot_freq_sinh();
    rx_frame.message_with_preamble.freq_shift(freq_shift);
    rx_frame.message_with_preamble.cp_freq_sinh();
    rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.ofdm_preamble.data(), rx_frame.preamble.size);

    write_complex_to_file("data/row.bin", rx_frame.buf.begin()+rx_frame.t2sin.size, rx_frame.buf.end());
    write_complex_to_file("data/source.bin", tx_frame.buf.begin()+tx_frame.t2sin.size, tx_frame.buf.end());

    auto constell = rx_frame.message_with_preamble.fft();

    auto phases = rx_frame.preamble.phase_shift();
    write_complex_to_file("data/phases.bin", phases);

    for (int i = 0; i < constell.size(); i++){
        constell[i] /= phases[i%phases.size()];
    }
    
    write_complex_to_file("data/constell.bin", constell);

    auto res_data = rx_frame.message.fft();

    for (int i = 0; i < res_data.size(); i++){
        res_data[i] /= phases[i%phases.size()];
    }
    
    auto res = rx_frame.message.Mod.demod(res_data);



    res.resize(rx_frame.usefull_size);

    double acc = 0.0;
    for (int i = 0; i < rx_frame.usefull_size; i++){
        acc += double(res[i]==origin_mes[i]);
    }
    acc /= rx_frame.usefull_size;
    
    std::cout<<"ACCURACY: "<< acc <<"\n";

    acc = 0.0;
    for (int i = 0; i < rx_frame.usefull_size; i++) {
        uint8_t diff = res[i] ^ origin_mes[i];
        for (int b = 0; b < 8; b++) {
            acc += ((diff >> b) & 1) == 0;
        }
    }
    acc /= (rx_frame.usefull_size * 8);

    std::cout << "Bit-level ACCURACY: " << acc << "\n";


    FILE* res_file = fopen("data.txt", "w");
    fwrite(res.data(), 1, res.size(), res_file);
    fclose(res_file);

    system("python3 python_code/ofdm.py");
    // print_vector(origin_mes);
    // print_vector(res);


    // Py_Initialize();
    // PyRun_SimpleString(
    //     "import matplotlib.pyplot as plt\n"
    //     "import numpy as np\n"

    //     "arr = np.fromfile('data/data.bin', dtype = np.float64)\n"
    //     "arr = arr[::2] + 1j * arr[1::2]\n"
    //     "#arr /= np.max(np.abs(arr))\n"
    //     "plt.plot(np.abs(arr))\n"
    //     "plt.title('data form sdr')\n"
    //     "plt.show()\n"

    //     // "corr = np.fromfile('data/t2_sin_corr.bin', dtype = np.float64)\n"
    //     // "#corr /= np.max(corr)\n"
    //     // "plt.plot(np.arange(0, arr.size, arr.size/corr.size), corr)\n"
    //     // "plt.title('t2sin cor')\n"
    //     // "plt.show()\n"

    //     "corr = np.fromfile('data/pr_corr.bin', dtype = np.float64)\n"
    //     "#corr /= np.max(corr)\n"
    //     "plt.plot(np.arange(0, arr.size, arr.size/corr.size), corr)\n"
    //     "plt.title('pr corr')\n"
    //     "plt.show()\n"

    //     // "arr = np.fromfile('data/row.bin', dtype = np.float64)\n"
    //     // "arr = arr[::2] + 1j * arr[1::2]\n"
    //     // "arr /= np.max(np.abs(arr))\n"
    //     // "plt.title('preamble + message')\n"

    //     // "arr1 = np.fromfile('data/source.bin', dtype = np.float64)\n"
    //     // "arr1 = arr1[::2] + 1j * arr1[1::2]\n"
    //     // "arr1 /= np.max(np.abs(arr1))\n"
    //     // "plt.plot(np.abs(arr1))\n"
    //     // "plt.plot(np.abs(arr))\n"
    //     // "plt.title('preamble + message')\n"
    //     // "plt.show()\n"

    //     // "plt.plot(np.abs(np.fft.fftshift(np.fft.fft(arr1))))\n"
    //     // "plt.plot(np.abs(np.fft.fftshift(np.fft.fft(arr))))\n"
    //     // "plt.show()\n"

    //     "arr = np.fromfile('data/constell.bin', dtype = np.float64)\n"
    //     "arr = arr[::2] + 1j*arr[1::2]\n"
    //     "plt.scatter(arr.real, arr.imag)\n"
    //     "plt.xlim(-1.1, 1.1)\n"
    //     "plt.ylim(-1.1, 1.1)\n"
    //     "plt.axis('equal')\n"
    //     "plt.show()\n"

    //     "arr = np.fromfile('data/phases.bin', dtype = np.float64)\n"
    //     "arr = arr[::2] + 1j*arr[1::2]\n"
    //     "plt.plot(np.angle(arr))\n"
    //     "plt.show()\n"

    // );   


    // Py_Finalize();



    
    return 0;
}


