#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>
#include "methods.h"

#define BLOCK_SIZE 64
#define MIN(a,b) (((a)<(b))?(a):(b))


// METODO DELLE POTENZE
double power_method_mpi(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_N = N / size;
    double *A_local = (double*)malloc(local_N * N * sizeof(double));
    
    // Il master (Rank 0) distribuisce le righe di A a tutti i nodi
    MPI_Scatter(A, local_N * N, MPI_DOUBLE, A_local, local_N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double *temp_local = (double*)malloc(local_N * sizeof(double));
    double *w_partial = (double*)malloc(N * sizeof(double));
    double *w_full = (double*)malloc(N * sizeof(double));

    // Il Master inizializza il vettore e lo invia a tutti (Bcast)
    if (rank == 0) {
        double norm_sq = 0.0;
        for(int i = 0; i < N; i++){     
            double val = rand();
            eigenvector[i] = val; 
            norm_sq += val * val;
        }
        double inv_norm = 1.0 / sqrt(norm_sq); 
        for(int i = 0; i < N; i++) eigenvector[i] *= inv_norm;
    }
    MPI_Bcast(eigenvector, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double lambda_old = 0.0;
    double lambda_new = 0.0;

    for(int iter = 0; iter < max_iter; iter++){      
        lambda_old = lambda_new;

        // ogni nodo calcola temp_local = A_local * v
        #pragma omp parallel for
        for(int i = 0; i < local_N; i++){
            double sum = 0.0; 
            for(int j = 0; j < N; j++){
                sum += A_local[i * N + j] * eigenvector[j];
            }
            temp_local[i] = sum;
        }

        #pragma omp parallel for
        for(int i = 0; i < N; i++){
            double sum = 0.0;
            for(int j = 0; j < local_N; j++){
                sum += A_local[j * N + i] * temp_local[j]; 
            }
            w_partial[i] = sum;
        }

        // la rete somma i w_partial di tutti e restituisce il w_full completo a tutti
        MPI_Allreduce(w_partial, w_full, N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        lambda_new = 0.0;
        double norm_w_sq = 0.0;
        for(int i = 0; i < N; i++){
            lambda_new += eigenvector[i] * w_full[i];
            norm_w_sq += w_full[i] * w_full[i];
        }

        if (iter > 0 && fabs(lambda_new - lambda_old) < tol) {
            if (rank == 0) printf("[*] Metodo delle potenze convergente in %d iterazioni.\n", iter);
            break;      
        }

        double inv_norm_w = 1.0 / sqrt(norm_w_sq); 
        for(int i = 0; i < N; i++) eigenvector[i] = w_full[i] * inv_norm_w;
    }

    free(A_local); free(temp_local); free(w_partial); free(w_full);
    return sqrt(lambda_new);
}


// ALGORITMO QR
void qr_algorithm_mpi(const double* restrict A, int N, int max_iter, double* restrict singular_values) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_N = N / size;
    double *A_local = (double*)malloc(local_N * N * sizeof(double));
    
    MPI_Scatter(A, local_N * N, MPI_DOUBLE, A_local, local_N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double *M_local = (double*)malloc(N * N * sizeof(double));
    double *M_full = (double*)malloc(N * N * sizeof(double));

    #pragma omp parallel for
    for(int i = 0; i < N * N; i++) M_local[i] = 0.0;

    // Ogni nodo calcola il SUO pezzo di matrice M usando Tiling a Blocchi
    #pragma omp parallel for schedule(dynamic)
    for(int i0 = 0; i0 < N; i0 += BLOCK_SIZE){
        for(int j0 = 0; j0 < N; j0 += BLOCK_SIZE){
            for(int k0 = 0; k0 < local_N; k0 += BLOCK_SIZE){
                for(int i = i0; i < MIN(i0 + BLOCK_SIZE, N); i++){
                    for(int k = k0; k < MIN(k0 + BLOCK_SIZE, local_N); k++){
                        double A_ki = A_local[k * N + i]; 
                        for(int j = j0; j < MIN(j0 + BLOCK_SIZE, N); j++){
                            M_local[i * N + j] += A_ki * A_local[k * N + j]; 
                        }
                    }
                }
            }
        }
    }


    MPI_Allreduce(M_local, M_full, N * N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    // Essendo il QR sequenziale, lasciamo riposare gli slave e facciamo calcolare solo il Master
    if (rank == 0) {
        double *Q = (double*)malloc(N * N * sizeof(double));
        double *R = (double*)malloc(N * N * sizeof(double));
        double *temp = (double*)malloc(N * N * sizeof(double));

        for (int iter = 0; iter < max_iter; iter++) {
            qr_decomposition_opt(M_full, Q, R, N);
            
            #pragma omp parallel for
            for(int i = 0; i < N * N; i++) temp[i] = 0.0;

            // R * Q a blocchi
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
            double *swap_ptr = M_full; M_full = temp; temp = swap_ptr;
        }
        for(int i = 0; i < N; i++) singular_values[i] = sqrt(M_full[i * N + i]);
        
        free(Q); free(R); free(temp);
    }

    free(A_local); free(M_local); free(M_full);
}