/*
DESCRIPTION:
		Experiments with thread synchronization

**************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<omp.h>

#define NUM_THREADS 2
static long num_steps = 100000;
double step;

/* main: process parameters */
int main(int argc, char *argv[]) {
    int i, nthreads, sum[NUM_THREADS];
    double pi;
    step = 1.0/(double)num_steps;
    omp_set_num_threads(NUM_THREADS);
    double begin = omp_get_wtime();

#pragma omp parallel
    {
        int i;
        double x;
        int nthrds = omp_get_num_threads();
        int id = omp_get_thread_num();
        if(id == 0){
            nthreads = nthrds;
        }
        for (i = id; i < num_steps; i=i+nthrds) {
            x = (i + 0.5) * step;
            sum[id] += 4.0 / (1.0 + x * x);
        }
    }
        for(i=0, pi = 0.0; i < nthreads;i++){
            pi += sum[i]*step;
        }
        double fim = omp_get_wtime();
        printf("Result: %f, time: %f\n",pi, fim - begin);
return 0;
}
