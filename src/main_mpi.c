#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include "io_utils.h"
#include "methods.h"
#include "verifier.h"

int main(int argc, char *argv[]) {
    int rank, size;
    
    // inizializzazione ambiente MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0) printf("Uso: mpirun -n <p> %s <dimensione_N> <file_matrice>\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    const char* filename = argv[2];

    if (N % size != 0) {
        if (rank == 0) printf("[!] Errore: La dimensione %d non e' perfettamente divisibile per i %d processi MPI.\n", N, size);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    double *matrix_1d = NULL;
    double *singular_values = NULL;
    
    // tutti i processi devono allocare l'autovettore, perché tutti ricevono i dati con MPI_Bcast
    double *eigenvector = (double*)malloc(N * sizeof(double));

    // solo il master legge il file e alloca le matrici pesanti
    if (rank == 0) {
        matrix_1d = (double*)malloc(N * N * sizeof(double));
        singular_values = (double*)malloc(N * sizeof(double));
        
        printf("\n======================================================\n");
        printf("[*] INIZIO ESECUZIONE DISTRIBUITA (MPI + OpenMP)\n");
        printf("[*] Nodi MPI: %d | Matrice: %dx%d\n", size, N, N);
        
        matrix_1d = (double*)malloc(N * N * sizeof(double));
        eigenvector = (double*)malloc(N * sizeof(double));
        singular_values = (double*)malloc(N * sizeof(double));
        
        printf("[*] Rank 0: Lettura del file %s...\n", filename);
        read_matrix(filename, matrix_1d, N);
    }

    // METODO DELLE POTENZE
    if (rank == 0) printf("\n[*] Esecuzione Metodo delle Potenze...\n");
    
    // allineiamo tutti i nodi prima di far partire il timer
    MPI_Barrier(MPI_COMM_WORLD);
    double start_pm = MPI_Wtime();
    
    double dominant_eigenvalue = power_method_mpi(matrix_1d, N, 1000, 1e-6, eigenvector);
    
    // aspettiamo che tutti abbiano finito per fermare il timer
    MPI_Barrier(MPI_COMM_WORLD);
    double end_pm = MPI_Wtime();

    if (rank == 0) {
        printf("[*] Autovalore dominante trovato: %f\n", dominant_eigenvalue);
    }

    // ==============================================================
    // 2. ALGORITMO QR ITERATIVO (Ibrido / Master-Only)
    // ==============================================================
    if (rank == 0) printf("\n[*] Esecuzione Algoritmo QR Iterativo...\n");
    
    MPI_Barrier(MPI_COMM_WORLD);
    double start_qr = MPI_Wtime();
    
    qr_algorithm_mpi(matrix_1d, N, 30, singular_values);

    MPI_Barrier(MPI_COMM_WORLD);
    double end_qr = MPI_Wtime();


    // STAMPA DEI RISULTATI
    if (rank == 0) {
        double time_pm = end_pm - start_pm;
        double time_qr = end_qr - start_qr;

        printf("\n======================================================\n");
        printf("[*] Tempo Metodo Potenze: %f secondi\n", time_pm);
        printf("[*] Tempo Algoritmo QR         : %f secondi\n", time_qr);
        printf("[*] TEMPO TOTALE (MPI+OpenMP)  : %f secondi\n", time_pm + time_qr);
        printf("======================================================\n");
        
        printf("[*] Primi 3 valori singolari trovati:\n");
        for (int i = 0; i < 3 && i < N; i++) {
            printf("    %d: %f\n", i+1, singular_values[i]);
        }
        
        verify_correctness(N, singular_values);
        
        free(matrix_1d);
        free(singular_values);
    }

    free(eigenvector);
    MPI_Finalize();
    return EXIT_SUCCESS;
}