#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "io_utils.h"
#include "methods.h"
#include "verifier.h"

// funzione per il tempo reale (Wall-clock time)
double get_current_time() {
#ifdef _OPENMP
    return omp_get_wtime();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#endif
}


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
        printf("Uso: %s <dimensione_N> <file_matrice> [naive | opt | blocked]\n", argv[0]);
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

    double start_pm, end_pm, start_qr, end_qr;

    // --- TEST ---
    if (strcmp(mode, "naive") == 0) {
        printf("\n[*] Modalita': NAIVE\n");

        double **matrix_2d = allocate_matrix_2d_main(N);
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                matrix_2d[i][j] = matrix_1d[i * N + j];
            }
        }

        // --- METODO DELLE POTENZE ---
        start_pm = get_current_time();
        dominant_eigenvalue = power_method_naive(matrix_2d, N, max_iterations_pm, tolerance, eigenvector);        
        end_pm = get_current_time();

        // --- ALGORITMO QR ---
        start_qr = get_current_time();
        qr_algorithm_naive(matrix_2d, N, qr_max_iterations, singular_values);        
        end_qr = get_current_time();

        double time_pm = end_pm - start_pm;
        double time_qr = end_qr - start_qr;

        printf("\n======================================================\n");
        printf("[*] Tempo Metodo Potenze : %f secondi\n", time_pm);
        printf("[*] Tempo Algoritmo QR   : %f secondi\n", time_qr);
        printf("[*] TEMPO TOTALE         : %f secondi\n", time_pm + time_qr);
        printf("======================================================\n\n");

        free_matrix_2d_main(matrix_2d, N);

    } else if (strcmp(mode, "opt") == 0) {
        printf("\n[*] Modalita': OPTIMIZED\n");

        // --- METODO DELLE POTENZE ---
        start_pm = get_current_time();
        dominant_eigenvalue = power_method_opt(matrix_1d, N, max_iterations_pm, tolerance, eigenvector);
        end_pm = get_current_time();

        // --- ALGORITMO QR ---
        start_qr = get_current_time();
        qr_algorithm_opt(matrix_1d, N, qr_max_iterations, singular_values);
        end_qr = get_current_time();

        double time_pm = end_pm - start_pm;
        double time_qr = end_qr - start_qr;

        printf("\n======================================================\n");
        printf("[*] Tempo Metodo Potenze : %f secondi\n", time_pm);
        printf("[*] Tempo Algoritmo QR   : %f secondi\n", time_qr);
        printf("[*] TEMPO TOTALE         : %f secondi\n", time_pm + time_qr);
        printf("======================================================\n\n");

    } else if (strcmp(mode, "blocked") == 0) {
        printf("\n[*] Modalita': OPTIMIZED con elaborazione a blocchi\n");

        // --- METODO DELLE POTENZE ---
        start_pm = get_current_time();
        dominant_eigenvalue = power_method_blocked(matrix_1d, N, max_iterations_pm, tolerance, eigenvector);
        end_pm = get_current_time();

        // --- ALGORITMO QR ---
        start_qr = get_current_time();
        qr_algorithm_blocked(matrix_1d, N, qr_max_iterations, singular_values);
        end_qr = get_current_time();

        double time_pm = end_pm - start_pm;
        double time_qr = end_qr - start_qr;

        printf("\n======================================================\n");
        printf("[*] Tempo Metodo Potenze : %f secondi\n", time_pm);
        printf("[*] Tempo Algoritmo QR   : %f secondi\n", time_qr);
        printf("[*] TEMPO TOTALE         : %f secondi\n", time_pm + time_qr);
        printf("======================================================\n\n");
    } else {
        printf("Errore: Argomento '%s' non valido. Usa 'naive', 'opt' o 'blocked'.\n", mode);
        free(matrix_1d);
        free(eigenvector);
        free(singular_values);
        return EXIT_FAILURE;
    }

    // stampiamo solo i primi 3 valori singolari
    printf("[*] Primi 3 valori singolari trovati:\n");
    for (int i = 0; i < 3 && i < N; i++) {
        printf("    %d: %f\n", i+1, singular_values[i]);
    }

    verify_correctness(N, singular_values);
    
    free(matrix_1d);
    free(eigenvector);
    free(singular_values);

    printf("\n[*] Esecuzione completata con successo.\n");
    return EXIT_SUCCESS;
}