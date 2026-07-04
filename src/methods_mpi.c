#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "methods.h"

// METODO DELLE POTENZE IBRIDO
double power_method_hybrid(const double *restrict local_A, int N, int local_N, int max_iter, double tol, double *restrict eigenvector, int rank, int size) {

  double *w = (double *)malloc(N * sizeof(double));
  double *local_w = (double *)malloc(N * sizeof(double));
  double *local_temp = (double *)malloc(local_N * sizeof(double));
  double *temp = (double *)malloc(N * sizeof(double));

  if (rank == 0) {      //inizializzazione: il processo 0 crea un vettore casuale e lo normalizza. 
                        //Successivamente viene broadcastato a tutti i processi.
    double norm_sq = 0.0;
    for (int i = 0; i < N; i++) {
      eigenvector[i] = (double)rand() / RAND_MAX;
      norm_sq += eigenvector[i] * eigenvector[i];
    }
    double inv_norm = 1.0 / sqrt(norm_sq);
    for (int i = 0; i < N; i++)
      eigenvector[i] *= inv_norm;
  }

  MPI_Bcast(eigenvector, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  double lambda_old = 0.0;
  double lambda_new = 0.0;

  for (int k = 0; k < max_iter; k++) {
    lambda_old = lambda_new;

    // FASE 1: calcolo locale di temp = A * eigenvector. Poiché ogni processo ha solo un pezzo di A, 
    // calcolerà solo un pezzo del vettore risultato (local_temp). 
    //I thread OpenMP si dividono i cicli for.
    #pragma omp parallel for
    for (int i = 0; i < local_N; i++) {
      double sum = 0.0;
      for (int j = 0; j < N; j++)
        sum += local_A[i * N + j] * eigenvector[j];
      local_temp[i] = sum;
    }
    
    //FASE 2: MPI (Gather). Tutti i pezzetti del vettore temp vengono uniti. Ora tutti i processi hanno il vettore temp completo.
    MPI_Allgather(local_temp, local_N, MPI_DOUBLE, temp, local_N, MPI_DOUBLE, MPI_COMM_WORLD);

    // FASE 3: calcolo locale di local_w = A^T * temp
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
      double sum = 0.0;
      for (int j = 0; j < local_N; j++) {
        sum += local_A[j * N + i] * temp[rank * local_N + j];
      }
      local_w[i] = sum;
    }

    // FASE 4: MPI Globale (Riduzione w)
    MPI_Allreduce(local_w, w, N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    // FASE 5: Aggiornamento
    lambda_new = 0.0;
    for (int i = 0; i < N; i++)
      lambda_new += eigenvector[i] * w[i];

    if (k > 0 && fabs(lambda_new - lambda_old) < tol) {
      if (rank == 0)
        printf("[*] Metodo delle potenze convergente in %d iterazioni.\n", k);
      break;
    }

    double norm_w_sq = 0.0;
    for (int i = 0; i < N; i++)
      norm_w_sq += w[i] * w[i];

    double inv_norm_w = 1.0 / sqrt(norm_w_sq);
    for (int i = 0; i < N; i++)
      eigenvector[i] = w[i] * inv_norm_w;
  }

  free(w);
  free(local_w);
  free(local_temp);
  free(temp);
  return sqrt(lambda_new);
}


// DECOMPOSIZIONE QR IBRIDA (Gram-Schmidt)
void qr_decomposition_hybrid(const double *restrict local_A, double *restrict local_Q, double *restrict global_R, int N, int local_N, int rank) {

  double *local_Q_T = (double *)malloc(N * local_N * sizeof(double));

  #pragma omp parallel for
  for (int i = 0; i < N * N; i++)
    global_R[i] = 0.0;

  // TRASPOSIZIONE LOCALE PREVENTIVA
  #pragma omp parallel for collapse(2)
  for (int i = 0; i < local_N; i++) {
    for (int j = 0; j < N; j++) {
      local_Q_T[j * local_N + i] = local_A[i * N + j];
    }
  }

  for (int i = 0; i < N; i++) {
    // 1. Calcolo Norma Parziale (Locale - OpenMP)
    double local_norm_sq = 0.0;
    #pragma omp parallel for reduction(+ : local_norm_sq)
    for (int k = 0; k < local_N; k++) {
      local_norm_sq += local_Q_T[i * local_N + k] * local_Q_T[i * local_N + k];
    }

    // 2. Sincronizzazione Norma (Globale - MPI)
    double global_norm_sq = 0.0;
    MPI_Allreduce(&local_norm_sq, &global_norm_sq, 1, MPI_DOUBLE, MPI_SUM,
                  MPI_COMM_WORLD);

    double R_ii = sqrt(global_norm_sq);
    global_R[i * N + i] = R_ii;

    // 3. Normalizzazione (Locale - OpenMP)
    double inv_R_ii = 1.0 / R_ii;
    #pragma omp parallel for
    for (int k = 0; k < local_N; k++) {
      local_Q_T[i * local_N + k] *= inv_R_ii;
    }

    // 4. Ortogonalizzazione sulle colonne successive
    for (int j = i + 1; j < N; j++) {

      // Prodotto scalare parziale (Locale - OpenMP)
      double local_dot = 0.0;
      #pragma omp parallel for reduction(+ : local_dot)
      for (int k = 0; k < local_N; k++) {
        local_dot += local_Q_T[i * local_N + k] * local_Q_T[j * local_N + k];
      }

      // Prodotto scalare globale (Globale - MPI)
      double global_dot = 0.0;
      MPI_Allreduce(&local_dot, &global_dot, 1, MPI_DOUBLE, MPI_SUM,
                    MPI_COMM_WORLD);
      global_R[i * N + j] = global_dot;

    // Aggiornamento (Locale - OpenMP)
    #pragma omp parallel for
      for (int k = 0; k < local_N; k++) {
        local_Q_T[j * local_N + k] -= (local_Q_T[i * local_N + k] * global_dot);
      }
    }
  }

  // RITRASPOSIZIONE LOCALE VERSO Q
  #pragma omp parallel for collapse(2)
  for (int i = 0; i < local_N; i++) {
    for (int j = 0; j < N; j++) {
      local_Q[i * N + j] = local_Q_T[j * local_N + i];
    }
  }

  free(local_Q_T);
}


// ALGORITMO QR IBRIDO
void qr_algorithm_hybrid(const double *restrict local_A, int N, int local_N, int max_iter, double *restrict singular_values, int rank, int size) {

  // 1. Calcolo di M = A^T * A. Essendo A distribuita per righe, sommiamo i
  // contributi locali
  double *global_M = (double *)malloc(N * N * sizeof(double));
  double *local_M_partial = (double *)calloc(N * N, sizeof(double));

#pragma omp parallel for
  for (int i = 0; i < N; i++) {
    for (int k = 0; k < local_N; k++) {
      double A_ki = local_A[k * N + i];
      for (int j = 0; j < N; j++) {
        local_M_partial[i * N + j] += A_ki * local_A[k * N + j];
      }
    }
  }

  // Tutti ottengono la matrice M globale completa tramite Allreduce
  MPI_Allreduce(local_M_partial, global_M, N * N, MPI_DOUBLE, MPI_SUM,
                MPI_COMM_WORLD);
  free(local_M_partial);

  // Mappiamo M nella sua porzione locale (vogliamo eseguire la QR distribuita)
  double *local_M = (double *)malloc(local_N * N * sizeof(double));
  for (int i = 0; i < local_N; i++) {
    for (int j = 0; j < N; j++) {
      local_M[i * N + j] = global_M[(rank * local_N + i) * N + j];
    }
  }

  double *local_Q = (double *)malloc(local_N * N * sizeof(double));
  double *global_R = (double *)malloc(N * N * sizeof(double));
  double *local_temp = (double *)malloc(local_N * N * sizeof(double));
  double *global_Q = (double *)malloc(N * N * sizeof(double));

#pragma omp parallel for
  for (int i = 0; i < local_N * N; i++) {
    local_temp[i] = 0.0; // Reset
  }

  for (int iter = 0; iter < max_iter; iter++) {

    // Decomposizione QR distribuita
    qr_decomposition_hybrid(local_M, local_Q, global_R, N, local_N, rank);

    // M_new = R * Q. Avendo Q distribuita per righe e R globale, ci serve
    // l'intera Q globale Raccogliamo i blocchi di Q in un'unica Q globale
    MPI_Allgather(local_Q, local_N * N, MPI_DOUBLE, global_Q, local_N * N,
                  MPI_DOUBLE, MPI_COMM_WORLD);

// temp = R * Q (ogni thread calcola solo le proprie righe)
#pragma omp parallel for
    for (int i = 0; i < local_N; i++) {
      int global_i = rank * local_N + i;
      for (int k = 0; k < N; k++) {
        double R_ik = global_R[global_i * N + k];
        for (int j = 0; j < N; j++) {
          local_temp[i * N + j] += R_ik * global_Q[k * N + j];
        }
      }
    }

    // Scambio puntatori locali
    double *swap_ptr = local_M;
    local_M = local_temp;
    local_temp = swap_ptr;

#pragma omp parallel for
    for (int i = 0; i < local_N * N; i++)
      local_temp[i] = 0.0; // Reset
  }

  // Estrazione dei valori singolari. Essendo la matrice distribuita per righe,
  // ogni processo possiede una porzione della diagonale principale.
  double *local_diag = (double *)malloc(local_N * sizeof(double));
  for (int i = 0; i < local_N; i++) {
    local_diag[i] = sqrt(fabs(local_M[i * N + (rank * local_N + i)]));
  }

  // Raccogliamo le porzioni di diagonale per formare il vettore completo dei
  // valori singolari
  MPI_Allgather(local_diag, local_N, MPI_DOUBLE, singular_values, local_N,
                MPI_DOUBLE, MPI_COMM_WORLD);

  free(global_M);
  free(local_M);
  free(local_Q);
  free(global_R);
  free(local_temp);
  free(global_Q);
  free(local_diag);
}