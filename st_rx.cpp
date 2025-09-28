#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <omp.h>
#include "OFDM/Frame.hpp"
#include <unistd.h>
#include <semaphore.h>
#include "sdr/sdr.hpp"
#include <iostream>



#define get_time(a) ((double)a.tv_sec+((double)a.tv_nsec)*1e-9)
#define time_dif(a,b) (get_time(b)-get_time(a))

#define BUF_NUM 2
#define MAX_ITER 4

using ci16 = std::complex<int16_t>; 
using cf64 = std::complex<double>; 

static struct timespec ts[16];
static int global_counter = 0;


//old code implementation
FRAME_FORM rx_frame("config/config.txt");
SDR rx_sdr(1, rx_frame.output_size, "config/config.txt");



void int16_double(ci16* in, cf64* out, size_t size){
    
    for(size_t i = 0; i < size; ++i)
        out[i] = cf64(in[i].real(), in[i].imag());
}



//sdr init phtread
ci16* row_sdr_buf;
cf64* sdr_buf;
cf64* sdr_but_ptr[BUF_NUM];

sem_t sdr_sem[2];
int sdr_stop_flag = 0;
pthread_t sdr_thread;

void* srd_reader(void* argv){

    uint sdr_counter = 0;

    for(int i = 0; i < MAX_ITER; ++i){   
        sem_wait(sdr_sem);

        if (sdr_stop_flag){
            return 0;
        }


        sdr_counter++;

        timespec_get(ts, TIME_UTC);
        for (int j = 0; j < 100000; j++){
            rx_sdr.recv(row_sdr_buf);
        }

        timespec_get(ts+1, TIME_UTC);

        sem_post(sdr_sem+1);

        //int16_double(row_sdr_buf, sdr_but_ptr[sdr_counter%BUF_NUM], rx_sdr.rx_buf_size);

        //timespec_get(ts+2, CLOCK_MONOTONIC);
    }
        
    return 0;
}


int main(){
    
    //sdr init phtread

        sem_init(sdr_sem  , 0, 0);
        sem_init(sdr_sem+1, 0, 0);

        sdr_buf     = (cf64*)malloc(rx_frame.from_sdr_buf.size()*sizeof(cf64)*BUF_NUM); 
        row_sdr_buf = (ci16*)malloc(rx_frame.from_sdr_buf.size()*sizeof(ci16)); 

        for (int i = 0; i < BUF_NUM; ++i){
            sdr_but_ptr[i] = sdr_buf+rx_frame.output_size+i*rx_frame.output_size;
        }

        pthread_create(&sdr_thread, NULL, srd_reader, NULL);

   
    sem_post(sdr_sem);

    sem_wait(sdr_sem+1);
    sdr_stop_flag = 1;
    sem_post(sdr_sem);

    pthread_join(sdr_thread, NULL);

    printf("%lf\n", time_dif(ts[0], ts[1]));

    free(row_sdr_buf);
    free(sdr_buf);
    

}

