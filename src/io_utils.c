#include <stdio.h>
#include <stdlib.h>
#include "io_utils.h"



// funzione per leggere la matrice dal file generato da Python
void read_matrix(const char* filename, double* matrix, int N) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Errore: impossibile aprire il file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // leggiamo N*N elementi
    for (int i = 0; i < N * N; i++) {
        if (fscanf(file, "%lf", &matrix[i]) != 1) {
            fprintf(stderr, "Errore durante la lettura dei dati al valore %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);
}