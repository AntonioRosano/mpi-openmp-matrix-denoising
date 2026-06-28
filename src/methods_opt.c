#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "methods.h"

// aggiunta del qualificatore 'restrict' per prevenire il Memory Aliasing e permette la vettorizzazione.
void qr_decomposition_opt(const double* restrict A, double* restrict Q, double* restrict R, int N) {
    
    // creiamo una trasposta temporanea Q_T in modo che le colonne diventano righe in memoria
    double *Q_T = (double*)malloc(N * N * sizeof(double));  //malloc per touch-by-all

    #pragma omp parallel    //creazione della regione parallela
    {

        #pragma omp for nowait    //divido i thread del pool per inizializzare la matrice R (e passo subito alla trasposizione)
        for(int i = 0; i < N * N; i++) {
            R[i] = 0.0;     //touch-by-all
        }

        // TRASPOSIZIONE PREVENTIVA
        #pragma omp for collapse(2)     //divido i thread del pool per trasporre la matrice Q
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                Q_T[j * N + i] = A[i * N + j];
            }
        }

        for (int i = 0; i < N; i++) {       //non parallelizzo questo ciclo perchè c'è il fenomeno delle dipendenze dei dati: ogni iterazione 
                                            //si aspetta il risultato dell'iterazione precedente.
                                            //ogni iterazione del ciclo verrà quindi eseguita da tutti i thread del pool che abbiamo creato.
            

            #pragma omp single              //questa sezione di codice la faccio eseguire da un solo thread, perchè essendo cicli con 
                                            //complessità O(N) (e con bassa intensità computazionale), non varrebbe la pena introdurre overhead per 
                                            //gestire la parallelizzazione
            {   
                // Calcolo della norma (con stride 1)
                double register norm_sq = 0.0; // accumulatore su registro
                for(int k = 0; k < N; k++){
                    norm_sq += Q_T[i * N + k] * Q_T[i * N + k];
                }        
                double R_ii = sqrt(norm_sq);
                R[i * N + i] = R_ii;

                // Normalizzazione (sostituiamo la divisione con una moltiplicazione (inverso))
                double inv_R_ii = 1.0 / R_ii; 
                for(int k = 0; k < N; k++){
                    Q_T[i * N + k] *= inv_R_ii;
                }
            }
            

            // Ortogonalizzazione
            #pragma omp for schedule(guided)        //qui invece divido il lavoro del ciclo tra i thread del pool
                                                    //uso guided perchè il carico di lavoro è decrescente
            for (int j = i + 1; j < N; j++) {
                
                // prodotto scalare tra due righe (entrambe Stride 1)
                double R_ij = 0.0; // accumulatore locale
                for(int k = 0; k < N; k++){
                    R_ij += Q_T[i * N + k] * Q_T[j * N + k];
                }
                R[i * N + j] = R_ij;

                // hoisting di R_ij fuori dal loop
                for(int k = 0; k < N; k++){
                    Q_T[j * N + k] -= (Q_T[i * N + k] * R_ij);
                }
            }
        }

        // ritrasponiamo Q_T indietro verso Q per rispettare l'interfaccia della funzione
        #pragma omp for collapse(2)
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                Q[i * N + j] = Q_T[j * N + i];
            }
        }
    }
    free(Q_T);
}

// =====================================================================

//anche qua usiamo restrict
void qr_algorithm_opt(const double* restrict A, int N, int max_iter, double* restrict singular_values) {

    //In questa funzione non abilito una regione parallela generica per via del fatto che viene chiamata la funzione qr_decomposition_opt.
    //Questo è risolvibile racchiudendo la chiamata dentro a un costrutto single e abilitando il parallelismo annidato.
    //Tuttavia, ho preferito aprire regioni parallele solo quando servivano (pagando un costo maggiore di overhead) per non dover gestire il parallelismo annidato (dato che in questo contesto il numero di core non è sufficiente a gestirlo efficientemente).
    
    double *M = (double*)malloc(N * N * sizeof(double));    //malloc per touch-by-all
    double *Q = (double*)malloc(N * N * sizeof(double));
    double *R = (double*)malloc(N * N * sizeof(double));
    double *temp = (double*)malloc(N * N * sizeof(double));
    
    #pragma omp parallel for
    for(int i = 0; i < N * N; i++) M[i] = 0.0;      //touch-by-all


    // calcolo di M = A^T * A
    // loop swapping: ordine i-k-j invece di i-j-k, che evita gli accessi per colonne
    #pragma omp parallel for        //ogni thread lavora su una riga diversa di M
    for(int i = 0; i < N; i++){
        for(int k = 0; k < N; k++){
            double A_ki = A[k * N + i]; // hoisting
            for(int j = 0; j < N; j++){
                M[i * N + j] += A_ki * A[k * N + j]; 
            }
        }
    }

    for (int iter = 0; iter < max_iter; iter++) {   //questa è una regione seriale
        
        qr_decomposition_opt(M, Q, R, N);   //viene chiamato solo dal thread master
        
        #pragma omp parallel for
        for(int i = 0; i < N * N; i++) temp[i] = 0.0;

        // temp = R * Q (loop swapping ordine i-k-j)
        #pragma omp parallel for
        for(int i = 0; i < N; i++){
            for(int k = 0; k < N; k++){
                double R_ik = R[i * N + k];
                for(int j = 0; j < N; j++){
                    temp[i * N + j] += R_ik * Q[k * N + j];
                }
            }
        }
       
        //scambio con puntatori
        double *swap_ptr = M;
        M = temp;
        temp = swap_ptr;
    }
        
    // estrazione - non vale la pena parallelizzare dato che il ciclo è O(N)
    for(int i = 0; i < N; i++){
        singular_values[i] = sqrt(M[i * N + i]);
    }
    
    
    free(M); free(Q); free(R); free(temp);
}

// =====================================================================

double power_method_opt(const double* restrict A, int N, int max_iter, double tol, double* restrict eigenvector) {

    double *w = (double*)malloc(N * sizeof(double));
    double *temp = (double*)malloc(N * sizeof(double));

    #pragma omp parallel for
    for(int i=0;i<N;i++){
        w[i] = 0.0;
        temp[i] = 0.0;
    }


    double norm_sq = 0.0;
    for(int i = 0; i < N; i++){     //lascio il codice seriale perchè è un ciclo a complessità O(N)
        double val = rand();
        eigenvector[i] = val; 
        norm_sq += val * val;
    }
    
    double inv_norm = 1.0 / sqrt(norm_sq); // pre-calcolo dell'inverso
    for(int i = 0; i < N; i++){     //lascio il codice seriale perchè è un ciclo a complessità O(N)
        eigenvector[i] *= inv_norm;
    }
    
    double lambda_old = 0.0;
    double lambda_new = 0.0;

    for(int k = 0; k < max_iter; k++){      //non apro la regione parallela per via della dipendenza tra i dati nelle iterazioni
        lambda_old = lambda_new;

        // Step 1: temp = A * eigenvector
        #pragma omp parallel
        {
            #pragma omp for
            for(int i = 0; i < N; i++){
                double register sum = 0.0; // accumulatore su registro
                for(int j = 0; j < N; j++){
                    sum += A[i * N + j] * eigenvector[j];
                }
                temp[i] = sum;
            }

            #ifndef _OPENMP
                // Step 2: w = A^T * temp
                for(int i = 0; i < N; i++) w[i] = 0.0;

                // Loop Swapping
                for(int j = 0; j < N; j++){
                    double t_j = temp[j]; // hoisting
                    for(int i = 0; i < N; i++){
                        w[i] += A[j * N + i] * t_j; 
                    }
                }
            #endif

            #ifdef _OPENMP
                // Step 2: w = A^T * temp
                #pragma omp for
                for(int i = 0; i < N; i++){
                    double register sum = 0.0;
                    for(int j = 0; j < N; j++){
                        sum += A[j * N + i] * temp[j]; // Lettura per colonne
                    }
                    w[i] = sum;
                }
            #endif
        }

        lambda_new = 0.0;
        for(int i = 0; i < N; i++){
            lambda_new += eigenvector[i] * w[i];
        }

        if (k > 0 && fabs(lambda_new - lambda_old) < tol) {
            printf("[*] Metodo delle potenze convergente in %d iterazioni.\n", k);
            break;      //il break è fuori dalla regione parallela ed è legale
        }

        double norm_w_sq = 0.0;
        for(int i = 0; i < N; i++){
            norm_w_sq += w[i] * w[i];
        }
        
        double inv_norm_w = 1.0 / sqrt(norm_w_sq); // inversione per evitare la divisione

        for(int i = 0; i < N; i++){     //non parallelizzo perchè è O(N)
            eigenvector[i] = w[i] * inv_norm_w;
        }
    }
        
    free(w);
    free(temp);
    return sqrt(lambda_new);
}