#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "io_utils.h"
#include "methods.h"


double** allocate_matrix_2d_main(int N) {
    double **mat = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        mat[i] = (double *)malloc(N * sizeof(double));
    }
    return mat;
}

void free_matrix_2d_main(double **mat, int N) {
    for (int i = 0; i < N; i++) {
        free(mat[i]);
    }
    free(mat);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <dimensione_N> <file_matrice> [naive | opt]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    const char* filename = argv[2];
    const char* mode = argv[3];

    printf("[*] Allocazione memoria per matrice %dx%d\n", N, N);

    double* matrix_1d = (double*)malloc(N * N * sizeof(double));
    if (matrix_1d == NULL) {
        fprintf(stderr, "Errore: Memoria insufficiente!\n");
        return EXIT_FAILURE;
    }

    printf("[*] Lettura del file %s\n", filename);
    read_matrix(filename, matrix_1d, N);

    // test di lettura
    printf("[*] Valore in basso a destra matrix[%d][%d] = %f\n", N-1, N-1, matrix_1d[(N-1)*N + (N-1)]);

    // allocazione degli array di output comuni
    double* eigenvector = (double*)malloc(N * sizeof(double));
    double* singular_values = (double*)malloc(N * sizeof(double));
    
    if (eigenvector == NULL || singular_values == NULL) {
        fprintf(stderr, "Errore allocazione array di output\n");
        return EXIT_FAILURE;
    }

    // parametri degli algoritmi
    int max_iterations_pm = 1000;
    double tolerance = 1e-6;
    int qr_max_iterations = 30;
    double dominant_eigenvalue = 0.0;

    // variabili per il cronometro
    clock_t start, end;
    double cpu_time_used;

    // --- TEST ---
    if (strcmp(mode, "naive") == 0) {
        printf("\n[*] Modalita': NAIVE (Array 2D, Stride elevato)\n");

        double **matrix_2d = allocate_matrix_2d_main(N);
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                matrix_2d[i][j] = matrix_1d[i * N + j];
            }
        }

        start = clock();

        printf("[*] Esecuzione Metodo delle Potenze...\n");
        dominant_eigenvalue = power_method_naive(matrix_2d, N, max_iterations_pm, tolerance, eigenvector);
        printf("[*] Autovalore dominante trovato: %f\n", dominant_eigenvalue);

        printf("\n[*] Esecuzione Algoritmo QR Iterativo...\n");
        qr_algorithm_naive(matrix_2d, N, qr_max_iterations, singular_values);

        end = clock();

        free_matrix_2d_main(matrix_2d, N);

    } else if (strcmp(mode, "opt") == 0) {
        printf("\n[*] Modalita': OPTIMIZED (Array 1D, Loop Swapping, SIMD)\n");

        start = clock();

        printf("[*] Esecuzione Metodo delle Potenze...\n");
        dominant_eigenvalue = power_method_opt(matrix_1d, N, max_iterations_pm, tolerance, eigenvector);
        printf("[*] Autovalore dominante trovato: %f\n", dominant_eigenvalue);

        printf("\n[*] Esecuzione Algoritmo QR Iterativo...\n");
        qr_algorithm_opt(matrix_1d, N, qr_max_iterations, singular_values);

        end = clock();

    } else if (strcmp(argv[3], "blocked") == 0) {
        printf("\n[*] Modalita': OPTIMIZED con blocchi (Array 1D, Loop Swapping, SIMD, elaborazione a blocchi)\n");

        start = clock();

        printf("[*] Esecuzione Metodo delle Potenze...\n");
        dominant_eigenvalue = power_method_blocked(matrix_1d, N, max_iterations_pm, tolerance, eigenvector);
        printf("[*] Autovalore dominante trovato: %f\n", dominant_eigenvalue);

        printf("\n[*] Esecuzione Algoritmo QR Iterativo...\n");
        qr_algorithm_blocked(matrix_1d, N, qr_max_iterations, singular_values);

        end = clock();
    } else {
        printf("Errore: Argomento '%s' non valido. Usa 'naive' o 'opt'.\n", mode);
        free(matrix_1d);
        free(eigenvector);
        free(singular_values);
        return EXIT_FAILURE;
    }

    // risultati
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("\n======================================================\n");
    printf("[*] Tempo di calcolo netto (%s): %f secondi\n", mode, cpu_time_used);
    printf("======================================================\n\n");

    // stampiamo solo i primi 3 valori singolari
    printf("[*] Primi 3 valori singolari trovati:\n");
    for (int i = 0; i < 3 && i < N; i++) {
        printf("    %d: %f\n", i+1, singular_values[i]);
    }

    free(matrix_1d);
    free(eigenvector);
    free(singular_values);

    printf("\n[*] Esecuzione completata con successo.\n");
    return EXIT_SUCCESS;
}