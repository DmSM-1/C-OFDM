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
#include <sstream>

#define TIME_TRACE


#define get_time(a) ((double)a.tv_sec+((double)a.tv_nsec)*1e-9)
#define time_dif(a,b) (get_time(b)-get_time(a))

#ifdef TIME_TRACE
#define TIME_TRACE_POINT(idx) timespec_get(ts+idx, TIME_UTC)
#define PRINT_LOG_TIME(str, t) s << str << ":" << time_dif(ts[t-1], ts[t]) << " "
#define PRINT_LOG_VAL(str, val) s << str << ":" << val << " "
#define PRINT_LOG_NL s << '\n'
#define PRINT_ITER_TIME(t) {TIME_TRACE_POINT(t); s<<"TIME:"<<time_dif(ts[1],ts[t]);}
#else
#define TIME_TRACE_POINT(idx) ;
#define PRINT_LOG(str, t) ;
#define PRINT_LOG_NL ;
#define PRINT_ITER_TIME(t) ;
#define PRINT_LOG_VAL(str, val) ;
#endif


static struct timespec ts[16];

std::complex<int16_t>* buf[2];


FRAME_FORM rx_frame("config/config.txt");
MAC mac(1, 0, rx_frame.usefull_size);
SDR rx_sdr(1, rx_frame.output_size, "config/config.txt");

sem_t sdr_sem[2];
pthread_t sdr_thread;

void* srd_reader(void* argv){
    int sdr_buf_num = 0;
    for(int i = 0; i < 1000; ++i){
        sem_wait(sdr_sem);
        rx_sdr.recv(buf[sdr_buf_num%2]);
        sem_post(sdr_sem+1);
        sdr_buf_num++;
    }
}


int buf_num = 0;
std::stringstream s;


void buf_update(){

    TIME_TRACE_POINT(4);
        sem_wait(sdr_sem+1);
        sem_post(sdr_sem);
    TIME_TRACE_POINT(5);
    PRINT_LOG_TIME("SDR", 5);

    TIME_TRACE_POINT(6);
        memcpy(
            rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
            buf[buf_num%2], 
            rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
        );
        buf_num++;
        rx_frame.form_int16_to_double();
    TIME_TRACE_POINT(7);
    PRINT_LOG_TIME("CONVERT", 7);
}


int main(){

    buf[0] = (std::complex<int16_t>*)malloc(rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>));
    buf[1] = (std::complex<int16_t>*)malloc(rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>));

    pthread_create(&sdr_thread, NULL, srd_reader, NULL);

    sem_post(sdr_sem);

    sem_wait(sdr_sem+1);
    sem_post(sdr_sem);
    memcpy(
        rx_frame.from_sdr_int16_buf.data()+rx_frame.output_size, 
        buf[buf_num%2], 
        rx_sdr.rx_buf_size*sizeof(std::complex<int16_t>)
    );
    buf_num++;

    rx_frame.form_int16_to_double();

    int pos = 0;
    int preamble_begin;
    int threshold = rx_frame.from_sdr_buf.size()-rx_frame.output_size;

    FILE* res_file = fopen("Res.wav", "wb");

    timespec_get(ts, TIME_UTC);
    int frame_counter = 0;
    int total_frame_counter = 0;

    int cycles = rx_frame.config["iterations"];

    for(int i = 0; i < cycles; i++){

        TIME_TRACE_POINT(1);
        PRINT_LOG_VAL("ITER", i);
        PRINT_LOG_TIME("GLOBAL", 1);
        
        TIME_TRACE_POINT(2);
        pos = rx_frame.t2sin.find_t2sin(rx_frame.from_sdr_buf, pos);
        TIME_TRACE_POINT(3);
        PRINT_LOG_TIME("T2SIN", 3);

        if (pos == -1){
            pos = rx_frame.output_size;
            buf_update();
            frame_counter = 0;

            PRINT_ITER_TIME(14);
            PRINT_LOG_NL;
            continue;
        }

        if (pos >= threshold){
            pos -= threshold;
            memcpy(
                rx_frame.from_sdr_int16_buf.data(), 
                rx_frame.from_sdr_int16_buf.data()+threshold, 
                rx_frame.output_size*sizeof(std::complex<int16_t>)
            );
            buf_update();
            frame_counter = 0;
        }

        preamble_begin = rx_frame.preamble.find_preamble(rx_frame.from_sdr_buf, pos)+1;

        if (preamble_begin < -2){
            pos += rx_frame.message.size;

            PRINT_ITER_TIME(14);
            PRINT_LOG_NL;
            continue;
        }

        pos = preamble_begin;
        
        if (pos == -1){
            pos = rx_frame.output_size;
            buf_update();
            frame_counter = 0;

            PRINT_ITER_TIME(14);
            PRINT_LOG_NL;
            continue;
        }

        if (pos >= threshold+rx_frame.t2sin.size){
            pos -= threshold;
            memcpy(
                rx_frame.from_sdr_int16_buf.data(), 
                rx_frame.from_sdr_int16_buf.data()+threshold, 
                rx_frame.output_size*sizeof(std::complex<int16_t>)
            );
            buf_update();
            frame_counter = 0;
        }
        frame_counter++;

        memcpy(
            rx_frame.buf.data()+rx_frame.t2sin.size, 
            rx_frame.from_sdr_buf.data()+pos, 
            (rx_frame.output_size-rx_frame.t2sin.size)*sizeof(complex_double)
        );

        pos += rx_frame.message.size;
        
        TIME_TRACE_POINT(8);
        double freq_shift = rx_frame.preamble.pilot_freq_sinh();
        rx_frame.message_with_preamble.freq_shift(freq_shift);
        TIME_TRACE_POINT(9);
        PRINT_LOG_TIME("PILOT_SINH", 9);

        rx_frame.message_with_preamble.cp_freq_sinh();
        rx_frame.message_with_preamble.pr_phase_sinh(rx_frame.preamble.ofdm_preamble.data(), rx_frame.preamble.size);
        TIME_TRACE_POINT(10);
        PRINT_LOG_TIME("FREQ_PHASE_SINH", 10);

        auto& chan_char = rx_frame.preamble.chan_char_lq();
        auto constell = rx_frame.message.fft();
        
        for (int j = 0; j < constell.size(); j++){
            constell[j] /= chan_char[j%chan_char.size()];
        }
        TIME_TRACE_POINT(11);
        PRINT_LOG_TIME("PFC", 11);

        auto res_ofdm = rx_frame.message.Mod.demod(constell);;
        auto res = mac.read(res_ofdm);
        TIME_TRACE_POINT(12);
        PRINT_LOG_TIME("MAC", 12);

        PRINT_LOG_VAL("SEQ", mac.input_seq_num);
        PRINT_LOG_VAL("DET", total_frame_counter);
        PRINT_LOG_VAL("FR_IN_BUF", frame_counter);

        
        total_frame_counter++;
        
        fwrite(res.data(), 1, res.size(), res_file);
        
        PRINT_ITER_TIME(14);
        PRINT_LOG_NL;
        
    }
    fclose(res_file);

    #ifdef TIME_TRACE
        FILE* log_file = fopen("LOG.txt", "w");
        fprintf(log_file, "%s", s.str().c_str());
        fclose(log_file);
    #endif
    
    free(buf[0]);
    free(buf[1]);

    std::cout<<s.str();

    return 0;
}


