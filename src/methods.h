#ifndef METHODS_H
#define METHODS_H

// firme per la versione naive
void qr_algorithm_naive(double** A, int N, int max_iter, double* singular_values);
double power_method_naive(double** A, int N, int max_iter, double tol, double* eigenvector);

// firme per la versione ottimizzata
void qr_decomposition_opt(const double* restrict A, double* restrict Q, double* restrict R, int N);
void qr_algorithm_opt(const double* restrict A, int N, int max_iter, double* restrict singular_values);
double power_method_opt(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector);

//firme per la versione ottimizzata a blocchi
void qr_decomposition_blocked(const double* restrict A, double* restrict Q, double* restrict R, int N);
void qr_algorithm_blocked(const double* restrict A, int N, int max_iter, double* restrict singular_values);
double power_method_blocked(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector);

#endif