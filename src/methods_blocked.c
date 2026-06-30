#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "methods.h"

// dimensioni del blocco (64x64 double = 32KB, ottimo per cache L1)
#define BLOCK_SIZE 64

#define MIN(a,b) (((a)<(b))?(a):(b))


void qr_decomposition_blocked(const double* restrict A, double* restrict Q, double* restrict R, int N) {
    qr_decomposition_opt(A, Q, R, N);
}

// nel metodo delle potenze, il tiling non serve
double power_method_blocked(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector) {
    return power_method_opt(A, N, max_iter, tol, eigenvector);
}

// implementazione a blocchi
void qr_algorithm_blocked(const double* restrict A, int N, int max_iter, double* restrict singular_values) {
    
    double *M = (double*)malloc(N * N * sizeof(double));
    double *Q = (double*)malloc(N * N * sizeof(double));
    double *R = (double*)malloc(N * N * sizeof(double));
    double *temp = (double*)malloc(N * N * sizeof(double));
    

    #pragma omp parallel for
    for(int i = 0; i < N * N; i++) M[i] = 0.0;      


    // calcolo di M = A^T * A
    // Parallelizziamo sui blocchi esterni: 'i0' garantisce che ogni thread
    // gestisca un gruppo di righe indipendente
    #pragma omp parallel for schedule(dynamic)
    for(int i0 = 0; i0 < N; i0 += BLOCK_SIZE){
        for(int j0 = 0; j0 < N; j0 += BLOCK_SIZE){
            for(int k0 = 0; k0 < N; k0 += BLOCK_SIZE){
                
                for(int i = i0; i < MIN(i0 + BLOCK_SIZE, N); i++){
                    for(int k = k0; k < MIN(k0 + BLOCK_SIZE, N); k++){
                        double A_ki = A[k * N + i];
                        for(int j = j0; j < MIN(j0 + BLOCK_SIZE, N); j++){
                            M[i * N + j] += A_ki * A[k * N + j]; 
                        }
                    }
                }

            }
        }
    }

    for (int iter = 0; iter < max_iter; iter++) {   
        
        qr_decomposition_blocked(M, Q, R, N);   
        
        #pragma omp parallel for
        for(int i = 0; i < N * N; i++) temp[i] = 0.0;

        // calcolo temp = R * Q
        #pragma omp parallel for schedule(dynamic)
        for(int i0 = 0; i0 < N; i0 += BLOCK_SIZE){
            for(int k0 = 0; k0 < N; k0 += BLOCK_SIZE){
                for(int j0 = 0; j0 < N; j0 += BLOCK_SIZE){
                    
                    for(int i = i0; i < MIN(i0 + BLOCK_SIZE, N); i++){
                        for(int k = k0; k < MIN(k0 + BLOCK_SIZE, N); k++){
                            double R_ik = R[i * N + k];
                            for(int j = j0; j < MIN(j0 + BLOCK_SIZE, N); j++){
                                temp[i * N + j] += R_ik * Q[k * N + j];
                            }
                        }
                    }

                }
            }
        }
       
        double *swap_ptr = M;
        M = temp;
        temp = swap_ptr;
    }
        
    for(int i = 0; i < N; i++){
        singular_values[i] = sqrt(M[i * N + i]);
    }
    
    free(M); free(Q); free(R); free(temp);
}