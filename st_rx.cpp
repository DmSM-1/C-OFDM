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

using c16 = std::complex<int16_t>; 

static struct timespec ts[16];
static int global_counter = 0;


//old code implementation
FRAME_FORM rx_frame("config/config.txt");
SDR rx_sdr(1, rx_frame.output_size, "config/config.txt");


//sdr init phtread
sem_t sdr_sem;
c16* sdr_buf;
c16* sdr_but_ptr[BUF_NUM];
pthread_t sdr_thread;
volatile int sdr_stop_flag = 0;

void* srd_reader(void* argv){

    uint sdr_counter = 0;

    for(int i = 0; i < MAX_ITER; ++i){   
        sem_wait(&sdr_sem);

        if (sdr_stop_flag)
            return 0;

        sdr_counter++;
        rx_sdr.recv(sdr_but_ptr[(sdr_counter)%BUF_NUM]);
    }
        
    return 0;
}


int main(){
    
    //sdr init phtread

        sem_init(&sdr_sem, 0, 0);

        sdr_buf = (c16*)malloc(rx_frame.from_sdr_buf.size()*sizeof(c16)*BUF_NUM); 

        for (int i = 0; i < BUF_NUM; ++i){
            sdr_but_ptr[i] = sdr_buf+rx_frame.output_size+i*rx_frame.output_size;
        }

        pthread_create(&sdr_thread, NULL, srd_reader, NULL);




        
    sem_post(&sdr_sem);

    
    sdr_stop_flag = 1;
    sem_post(&sdr_sem);
    pthread_join(sdr_thread, NULL);

    free(sdr_buf);
    

}

