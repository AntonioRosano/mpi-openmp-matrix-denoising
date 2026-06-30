#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "verifier.h"

int compare_desc(const void *a, const void *b) {
    double diff = (*(double*)b - *(double*)a);
    if (diff > 0) return 1;
    if (diff < 0) return -1;
    return 0;
}

void verify_correctness(int N, double* my_singular_values) {
    char gt_filename[256];
    sprintf(gt_filename, "data/groundtruth_S_%d.txt", N);

    FILE* file = fopen(gt_filename, "r");
    if (!file) {
        printf("[!] File ground truth non trovato: %s\n", gt_filename);
        return;
    }

    qsort(my_singular_values, N, sizeof(double), compare_desc);

    double max_relative_error = 0.0;
    double avg_relative_error = 0.0;
    double gt_val;
    int valid = 0;

    for (int i = 0; i < N; i++) {
        if (fscanf(file, "%lf", &gt_val) != 1) break;
        double diff = fabs(my_singular_values[i] - gt_val);
        double rel_error = (gt_val > 1e-12) ? (diff / gt_val) : diff;

        if (rel_error > max_relative_error) max_relative_error = rel_error;
        avg_relative_error += rel_error;
        valid++;
    }
    
    if (valid > 0) avg_relative_error /= valid;
    fclose(file);

    printf("\n======================================================\n");
    printf("[*] VERIFICA CORRETTEZZA (LAPACK Ground-Truth)\n");
    printf("[*] Errore Relativo Medio   : %e\n", avg_relative_error);
    printf("[*] Errore Relativo Massimo : %e\n", max_relative_error);
    printf("======================================================\n");
}