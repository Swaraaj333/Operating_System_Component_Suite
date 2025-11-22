#ifndef SIMPLE_MULTITHREADER_H
#define SIMPLE_MULTITHREADER_H


#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <functional>
#include <vector>
#include <cstdlib>
#include <string.h>


//./matrix  <numThreads>   <rows>   <cols>
//./vector  <numThreads>   <size>

//time difference calculation
static inline double time_difference(struct timeval* start, struct timeval* end){
    double sec_diff = end->tv_sec -start->tv_sec;
    double usec_diff = end->tv_usec -start->tv_usec;
    return sec_diff+(usec_diff/1000000.0);
}


//1d parallel for loop


//struct holds arg for 1d thread
typedef struct {
    int i_start;
    int i_end;
    //loop body 
    std::function<void(int)> lambda_fn;
}threadarg_1d_loop;

//this is thread function for 1d loop,for executing chunk in 1d loop
static void* thread_1d_loop_function(void* arg) {
    threadarg_1d_loop* data=(threadarg_1d_loop*)arg;
    
    //this executes the loop body for a assigned range
    for(int i = data->i_start; i < data->i_end; i++) {
        data->lambda_fn(i);
    }
    
    //fress the thread arg memory
    delete data;
    return NULL;
}


//this is for 1d loops
static inline void parallel_for(int low, int high,std::function<void(int)> &&lambda,int numb_thread){
    // no iteration
    if(high <= low) return;

    // atleast 1 thread
    if(numb_thread < 1){ 
        numb_thread = 1;
    }

    // total iterations
    int tot_work =high - low; 

    // this avoids more thread than the given tasks
    if(numb_thread > tot_work) { 
        numb_thread =tot_work;
    }

    //size of base chunk
    int chunks_siz = tot_work/numb_thread;

    //iterations which are leftover
    int excess_chunk = tot_work%numb_thread;

    //this is the wroker thread
    std::vector<pthread_t> thread_ids(numb_thread - 1);

    //get the time of start
    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);


    //this thread takes care of the first chunk
    int firstchunk_size = chunks_siz;
    if(excess_chunk>0) {
        firstchunk_size += 1;
    }

    int firststart = low;
    int firstend = firststart + firstchunk_size;

    for(int i = firststart;i<firstend;i++) {
        //runs the loop body
        lambda(i);
    }
    
    //this starts the next chunk
    int curr_start = firstend;

    //this make worker threads for the remaining chunks
    for(int t = 1; t < numb_thread; t++){
        int this_chunk = chunks_siz;

        if(t < excess_chunk){
            this_chunk += 1;
        }

        //this makes the arguments
        threadarg_1d_loop* arg = new threadarg_1d_loop();
        arg->i_start = curr_start;
        arg->i_end   = curr_start + this_chunk;
        arg->lambda_fn = lambda;

        //moving to the next chunk start

        curr_start += this_chunk;

        int rc_1d = pthread_create(&thread_ids[t-1], NULL,thread_1d_loop_function, arg);
        if(rc_1d != 0){
            fprintf(stderr, "unable to create pthread %s\n", strerror(rc_1d));
            exit(1);
        }
    }


    //waiting for all the worker threads to complete their work
    for(pthread_t& tid : thread_ids) {
        int rc_1d = pthread_join(tid, NULL);
        if(rc_1d != 0) {
            fprintf(stderr, "unable to perform pthread join %s\n", strerror(rc_1d));
            exit(1);
        }
    }

    //this is the time of ending
    gettimeofday(&t_end, NULL);

    //prints the time
    printf("parallel for time = %f sec\n",time_difference(&t_start, &t_end));
}



//2d for loop

//this struct is for args for 2D threads
typedef struct {
    int row_i_start;
    int row_i_end;
    int col_j_start;
    int col_j_end;
    //this is the 2d loop body
    std::function<void(int,int)> lambda_fn;
} threadarg_2d_loop;


//thread func for executing block of rows in the 2d loop
static void* thread_2d_loop_function(void* arg) {
    threadarg_2d_loop* data = (threadarg_2d_loop*)arg;

    //this runs nested loops for rows
    for(int i = data->row_i_start; i < data->row_i_end; i++) {
        for(int j = data->col_j_start; j < data->col_j_end; j++) {
            data->lambda_fn(i, j);
        }
    }

    //freeing the thread arg memory
    delete data;
    return NULL;
}


// paralle for function for 2d loops 
static inline void parallel_for(int low1, int high1,int low2, int high2,std::function<void(int,int)> &&lambda,int numb_thread){
    
    //there are no rows
    if(high1 <= low1) return;
    //there are no columns
    if(high2 <= low2) return;

    // make sure there is at least 1 thead
    if(numb_thread < 1){
        numb_thread = 1;
    }

    //total rows to process
    int tot_rows = high1 - low1;

    //no of threads are limted to the number of rows
    if(numb_thread > tot_rows){
        numb_thread = tot_rows;
    }

    // total number of rows per thread
    int chunks_siz = tot_rows/numb_thread;
    // number of left over rows
    int excess_chunk = tot_rows%numb_thread;

    std::vector<pthread_t> thread_ids(numb_thread - 1);

    struct timeval t_start, t_end;
    //this records the time of starting
    gettimeofday(&t_start, NULL); 


    //this is the main thread that handles the first block of rows
    int firstchunk_size = chunks_siz;
    if(excess_chunk > 0) {
        firstchunk_size += 1;
    }

    int firststart = low1;
    int firstend = firststart + firstchunk_size;

    for(int i = firststart; i < firstend; i++) {
        for(int j = low2; j < high2; j++) {
            //this runs the loop body
            lambda(i, j);
        }
    }

    //this starts the next row block
    int curr_start = firstend;


    //this make the worker thread for the remaining row blocks
    for(int t = 1; t < numb_thread; t++) {
        int this_chunk = chunks_siz;
        if(t < excess_chunk) {
            this_chunk += 1;
        }

        //this make the arg for 2d loop
        threadarg_2d_loop* arg = new threadarg_2d_loop();
        arg->row_i_start = curr_start;
        arg->row_i_end = curr_start + this_chunk;
        arg->col_j_start = low2;
        arg->col_j_end = high2;
        arg->lambda_fn = lambda;

        //this updates the next row start
        curr_start += this_chunk;

        int rc_2d = pthread_create(&thread_ids[t-1], NULL,thread_2d_loop_function, arg);

        if(rc_2d != 0) {
            fprintf(stderr,"unable to create pthread %s\n", strerror(rc_2d));
            exit(1);
        }
    }


    //wait for all 2d loop worker threads
    for(pthread_t& tid : thread_ids) {
        int rc_2d = pthread_join(tid, NULL);
        if(rc_2d != 0) {
            fprintf(stderr,"unable to join pthread %s\n", strerror(rc_2d));
            exit(1);
        }
    }

    //thsi records the end time
    gettimeofday(&t_end,NULL);

    printf("parallel for 2d loop time = %f sec\n",time_difference(&t_start,&t_end));
}

#endif
