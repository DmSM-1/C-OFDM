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
#include "mac/mac_frame.hpp"
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>



#define get_time(a) ((double)a.tv_sec+((double)a.tv_nsec)*1e-9)
#define time_dif(a,b) (get_time(b)-get_time(a))

static struct timespec ts[16];

std::complex<int16_t>* buf[2];


FRAME_FORM rx_frame("config/config.txt");
MAC mac(1, 0, rx_frame.usefull_size);
SDR rx_sdr(1, rx_frame.output_size, "config/config.txt");

sem_t sdr_sem[2];
pthread_t sdr_thread;

void* srd_reader(void* argv){
    int buf_num = 0;
    for(int i = 0; i < 1000; ++i){
        sem_wait(sdr_sem);
        rx_sdr.recv(buf[buf_num%2]);
        // rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
        buf_num++;
        sem_post(sdr_sem+1);
    }
}



int main(){

    int buf_num = 0;
    
    buf[0] = (std::complex<int16_t>*)malloc(rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>));
    buf[1] = (std::complex<int16_t>*)malloc(rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>));

    pthread_create(&sdr_thread, NULL, srd_reader, NULL);

    // timespec_get(ts+2, TIME_UTC);
    sem_post(sdr_sem);
    sem_wait(sdr_sem+1);
    sem_post(sdr_sem);
    memcpy(
        rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
        buf[buf_num%2], 
        rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
    );
    buf_num++;
    // timespec_get(ts+3, TIME_UTC);

    // timespec_get(ts+6, TIME_UTC);
    rx_frame.form_int16_to_double();
    // timespec_get(ts+7, TIME_UTC);

    int pos = 0;
    int preamble_begin;
    int threshold = rx_frame.from_sdr_buf.size()-rx_frame.output_size;

    FILE* res_file = fopen("Res.wav", "wb");

    timespec_get(ts, TIME_UTC);
    int frame_counter = 0;
    int total_frame_counter = 0;

    for(int i = 0; i < 50000; i++){
        
        printf("\r%6d ", i);
        // timespec_get(ts+4, TIME_UTC);
        pos = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, pos);
        // timespec_get(ts+5, TIME_UTC);

        if (pos == -1){
            pos = rx_frame.output_size;

            // timespec_get(ts+2, TIME_UTC);
            // rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            sem_wait(sdr_sem+1);
            sem_post(sdr_sem);
            memcpy(
                rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
                buf[buf_num%2], 
                rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
            );
            buf_num++;
            // timespec_get(ts+3, TIME_UTC);

            frame_counter = 0;

            // timespec_get(ts+6, TIME_UTC);
            rx_frame.form_int16_to_double();
            // timespec_get(ts+7, TIME_UTC);

            continue;
        }

        if (pos >= threshold){
            memcpy(
                rx_frame.from_sdr_int16_buf.data(), 
                rx_frame.from_sdr_int16_buf.data()+threshold, 
                rx_frame.output_size*sizeof(std::complex<int16_t>));

            // timespec_get(ts+2, TIME_UTC);
            sem_wait(sdr_sem+1);
            sem_post(sdr_sem);
            memcpy(
                rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
                buf[buf_num%2], 
                rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
            );
            buf_num++;
            // timespec_get(ts+3, TIME_UTC);

            frame_counter = 0;

            // timespec_get(ts+6, TIME_UTC);
            rx_frame.form_int16_to_double();
            // timespec_get(ts+7, TIME_UTC);

            pos -= threshold;
        }

        preamble_begin = rx_frame.preamble.find_preamble(rx_frame.from_sdr_buf, pos)+1;

        if (preamble_begin < -2){
            pos += rx_frame.message.size;
            continue;
        }
        pos = preamble_begin;
        
        if (pos == -1){
            pos = rx_frame.output_size;

            // timespec_get(ts+2, TIME_UTC);
            // rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            sem_wait(sdr_sem+1);
            sem_post(sdr_sem);
            memcpy(
                rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
                buf[buf_num%2], 
                rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
            );
            buf_num++;
            // timespec_get(ts+3, TIME_UTC);

            frame_counter = 0;

            // timespec_get(ts+6, TIME_UTC);
            rx_frame.form_int16_to_double();
            // timespec_get(ts+7, TIME_UTC);

            continue;
        }

        if (pos >= threshold+rx_frame.t2sin.size){
            memcpy(
                rx_frame.from_sdr_int16_buf.data(), 
                rx_frame.from_sdr_int16_buf.data()+threshold, 
                rx_frame.output_size*sizeof(std::complex<int16_t>)
            );

            // timespec_get(ts+2, TIME_UTC);
            // rx_sdr.recv(rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size);
            sem_wait(sdr_sem+1);
            sem_post(sdr_sem);
            memcpy(
                rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
                buf[buf_num%2], 
                rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
            );
            buf_num++;
            // timespec_get(ts+3, TIME_UTC);

            frame_counter = 0;

            // timespec_get(ts+6, TIME_UTC);
            rx_frame.form_int16_to_double();
            // timespec_get(ts+7, TIME_UTC);

            pos -= threshold;
        }
        frame_counter++;

        memcpy(
            rx_frame.buf.data()+rx_frame.t2sin.size, 
            rx_frame.from_sdr_buf.data()+pos, 
            (rx_frame.output_size-rx_frame.t2sin.size)*sizeof(complex_double)
        );

        pos += rx_frame.message.size;
        
        double freq_shift = rx_frame.preamble.pilot_freq_sinh();
        rx_frame.message_with_preamble.freq_shift(freq_shift);

        rx_frame.message_with_preamble.cp_freq_sinh();
        rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.ofdm_preamble.data(), rx_frame.preamble.size);
        auto& chan_char = rx_frame.preamble.chan_char_lq();
        auto constell = rx_frame.message.fft();
        
        for (int j = 0; j < constell.size(); j++){
            constell[j] /= chan_char[j%chan_char.size()];
        }
        auto res_ofdm = rx_frame.message.Mod.demod(constell);;
        auto res = mac.read(res_ofdm);
        timespec_get(ts+1, TIME_UTC);

        printf("SEQ NUM:%6d %6d TIME: %1.6lf", 
            mac.input_seq_num, total_frame_counter++,
            time_dif(ts[0], ts[1])
        );
        
        fwrite(res.data(), 1, res.size(), res_file);
        
    }
    fclose(res_file);
    

    free(buf[0]);
    free(buf[1]);
    return 0;
}


