#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "methods.h"

// funzione per allocare una matrice 2D in modo naive (array di puntatori)
double** allocate_matrix_2d(int N) {
    double **mat = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        mat[i] = (double *)malloc(N * sizeof(double));
    }
    return mat;
}

// funzione per liberare una matrice 2D
void free_matrix_2d(double **mat, int N) {
    for (int i = 0; i < N; i++) {
        free(mat[i]);
    }
    free(mat);
}

// =====================================================================

void qr_decomposition_naive(double** A, double** Q, double** R, int N) {
    
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            R[i][j] = 0.0;
        }
    }

    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            Q[i][j] = A[i][j];
        }
    }

    for (int i = 0; i < N; i++) {
        
        // calcolo della norma (Stride N)
        for(int k = 0; k < N; k++){
            // scrittura diretta in memoria ad ogni iterazione (niente accumulatore locale)
            R[i][i] = R[i][i] + (Q[k][i] * Q[k][i]);
        }        
        R[i][i] = sqrt(R[i][i]);

        // normalizzazione: divisione lenta ripetuta N volte
        for(int k = 0; k < N; k++){
            Q[k][i] = Q[k][i] / R[i][i];
        }

        // ortogonalizzazione
        for (int j = i + 1; j < N; j++) {
            
            // prodotto scalare con stride elevato
            for(int k = 0; k < N; k++){
                R[i][j] = R[i][j] + (Q[k][i] * Q[k][j]);
            }

            // lettura multipla di R[i][j] dal loop interno
            for(int k = 0; k < N; k++){
                Q[k][j] = Q[k][j] - (Q[k][i] * R[i][j]);
            }
        }
    }
}

// =====================================================================

void qr_algorithm_naive(double** A, int N, int max_iter, double* singular_values) {
    
    double **M = allocate_matrix_2d(N);
    double **Q = allocate_matrix_2d(N);
    double **R = allocate_matrix_2d(N);
    double **temp = allocate_matrix_2d(N);
    
    // M = A^T * A calcolato in modo naive
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            M[i][j] = 0.0;
            for(int k = 0; k < N; k++){
                M[i][j] = M[i][j] + (A[k][i] * A[k][j]);
            }
        }
    }
    
    for (int iter = 0; iter < max_iter; iter++) {
        
        qr_decomposition_naive(M, Q, R, N);
        
        // Ricostruzione M = R * Q (ordine i-j-k)
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++){
                temp[i][j] = 0.0;
                for(int k = 0; k < N; k++){
                    temp[i][j] = temp[i][j] + (R[i][k] * Q[k][j]); // Q letto per colonne
                }
            }
        }

        for (int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                M[i][j] = temp[i][j];
            }
        }
    }
    
    // estrazione
    for(int i = 0; i < N; i++){
        singular_values[i] = sqrt(M[i][i]);
    }
    
    free_matrix_2d(M, N);
    free_matrix_2d(Q, N);
    free_matrix_2d(R, N);
    free_matrix_2d(temp, N);
}

// =====================================================================

double power_method_naive(double** A, int N, int max_iter, double tol, double* eigenvector) {

    double *w = (double*)malloc(N * sizeof(double));
    double *temp = (double*)malloc(N * sizeof(double));

    double norm_eig = 0.0;
    for(int i = 0; i < N; i++){
        eigenvector[i] = rand(); 
        norm_eig = norm_eig + (eigenvector[i] * eigenvector[i]);
    }
    norm_eig = sqrt(norm_eig);
    
    for(int i = 0; i < N; i++){
        eigenvector[i] = eigenvector[i] / norm_eig;
    }
    
    double lambda_old = 0.0;
    double lambda_new = 0.0;

    for(int k = 0; k < max_iter; k++){
        lambda_old = lambda_new;

        // 1: temp = A * eigenvector
        for(int i = 0; i < N; i++){
            temp[i] = 0.0;
            for(int j = 0; j < N; j++){
                temp[i] = temp[i] + (A[i][j] * eigenvector[j]);
            }
        }

        // 2: w = A^T * temp
        for(int i = 0; i < N; i++){
            w[i] = 0.0;
            for(int j = 0; j < N; j++){
                // Accesso per colonna (A[j][i]) -> alta probabilità di cache miss
                w[i] = w[i] + (A[j][i] * temp[j]);
            }
        }

        lambda_new = 0.0;
        for(int i = 0; i < N; i++){
            lambda_new = lambda_new + (eigenvector[i] * w[i]);
        }

        if (k > 0 && fabs(lambda_new - lambda_old) < tol) {
            break;
        }

        double norm_w = 0.0;
        for(int i = 0; i < N; i++){
            norm_w = norm_w + (w[i] * w[i]);
        }
        norm_w = sqrt(norm_w);

        for(int i = 0; i < N; i++){
            eigenvector[i] = w[i] / norm_w;
        }
    }
        
    free(w);
    free(temp);
    return sqrt(lambda_new);
}