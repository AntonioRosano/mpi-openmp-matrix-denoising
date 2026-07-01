#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io_utils.h"
#include "methods.h"
#include "verifier.h"

int main(int argc, char *argv[]) {
  int provided;

  // Inizializzazione ibrida: livello MPI_THREAD_FUNNELED (OpenMP calcola, il
  // Master comunica)
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

  if (provided < MPI_THREAD_FUNNELED) {
    printf("Errore: Il supporto MPI_THREAD_FUNNELED non è disponibile.\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 3) {
    if (rank == 0)
      printf("Uso: mpirun -np <proc> %s <dimensione_N> <file_matrice>\n",
             argv[0]);
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  int N = atoi(argv[1]);
  const char *filename = argv[2];

  if (N % size != 0) {
    if (rank == 0)
      printf("Errore: N (%d) deve essere perfettamente divisibile per il "
             "numero di processi MPI (%d)\n",
             N, size);
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  int local_N = N / size;
  double *matrix_1d = NULL;

  // Lettura file limitata al Master
  if (rank == 0) {
    matrix_1d = (double *)malloc(N * N * sizeof(double));
    if (matrix_1d == NULL) {
      fprintf(stderr, "Errore: Memoria insufficiente!\n");
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    printf("[*] Lettura del file %s (N=%d)\n", filename, N);
    read_matrix(filename, matrix_1d, N);
  }

  // Buffer locale (allocato con Touch-by-all policy nativamente da OpenMP in
  // background)
  double *local_A = (double *)malloc(local_N * N * sizeof(double));

  // Scatter: divide la matrice a strisce orizzontali
  MPI_Scatter(matrix_1d, local_N * N, MPI_DOUBLE, local_A, local_N * N,
              MPI_DOUBLE, 0, MPI_COMM_WORLD);

  if (rank == 0)
    free(matrix_1d);

  double *eigenvector = (double *)malloc(N * sizeof(double));
  double *singular_values = (double *)malloc(N * sizeof(double));
  double dominant_eigenvalue = 0.0;

  int max_iterations_pm = 1000;
  double tolerance = 1e-6;
  int qr_max_iterations = 30;

  // --- ESECUZIONE METODO DELLE POTENZE ---
  MPI_Barrier(MPI_COMM_WORLD);
  double start_pm = MPI_Wtime();
  dominant_eigenvalue =
      power_method_hybrid(local_A, N, local_N, max_iterations_pm, tolerance,
                          eigenvector, rank, size);
  MPI_Barrier(MPI_COMM_WORLD);
  double end_pm = MPI_Wtime();

  // --- ESECUZIONE ALGORITMO QR ---
  MPI_Barrier(MPI_COMM_WORLD);
  double start_qr = MPI_Wtime();
  qr_algorithm_hybrid(local_A, N, local_N, qr_max_iterations, singular_values,
                      rank, size);
  MPI_Barrier(MPI_COMM_WORLD);
  double end_qr = MPI_Wtime();

  // Output Globale
  if (rank == 0) {
    double time_pm = end_pm - start_pm;
    double time_qr = end_qr - start_qr;

    printf("\n======================================================\n");
    printf("[*] Autovalore Dominante Trovato : %f\n", dominant_eigenvalue);
    printf("[*] Tempo Metodo Potenze (Ibrido): %f secondi\n", time_pm);
    printf("[*] Tempo Algoritmo QR (Ibrido)  : %f secondi\n", time_qr);
    printf("[*] TEMPO TOTALE                 : %f secondi\n",
           time_pm + time_qr);
    printf("======================================================\n\n");

    printf("[*] Primi 3 valori singolari trovati:\n");
    for (int i = 0; i < 3 && i < N; i++) {
      printf("    %d: %f\n", i + 1, singular_values[i]);
    }
    verify_correctness(N, singular_values);
  }

  free(local_A);
  free(eigenvector);
  free(singular_values);

  MPI_Finalize();
  return EXIT_SUCCESS;
}