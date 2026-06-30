#include <stdio.h>
#include <stdlib.h>
#include "io_utils.h"

// funzione per leggere la matrice dal file generato da Python
void read_matrix(const char* filename, double* matrix, int N) {
    FILE* file = fopen(filename, "r"); // Modalità testo
    if (file == NULL) { perror("Errore apertura"); exit(1); }

    for (int i = 0; i < N * N; i++) {
        if (fscanf(file, "%lf", &matrix[i]) != 1) {
            fprintf(stderr, "Errore lettura elemento %d\n", i);
            exit(1);
        }
    }
    fclose(file);
}