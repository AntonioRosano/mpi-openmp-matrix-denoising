#ifndef METHODS_H
#define METHODS_H

// Naive
double** allocate_matrix_2d(int N);
void free_matrix_2d(double **mat, int N);
void qr_decomposition_naive(double** A, double** Q, double** R, int N);
void qr_algorithm_naive(double** A, int N, int max_iter, double* singular_values);
double power_method_naive(double** A, int N, int max_iter, double tol, double* eigenvector);

// Opt
void qr_decomposition_opt(const double* restrict A, double* restrict Q, double* restrict R, int N);
void qr_algorithm_opt(const double* restrict A, int N, int max_iter, double* restrict singular_values);
double power_method_opt(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector);

// Blocked
void qr_decomposition_blocked(const double* restrict A, double* restrict Q, double* restrict R, int N);
void qr_algorithm_blocked(const double* restrict A, int N, int max_iter, double* restrict singular_values);
double power_method_blocked(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector);

// MPI+OpenMP (Ibrido)
double power_method_mpi(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector);
void qr_algorithm_mpi(const double* restrict A, int N, int max_iter, double* restrict singular_values);

#endif