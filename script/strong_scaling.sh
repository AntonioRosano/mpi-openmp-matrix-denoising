#!/bin/bash

# Posizioniamoci sempre nella root del progetto, indipendentemente da dove viene lanciato lo script
cd "$(dirname "$0")/.." || exit 1

# Matrici da testare per lo strong scaling
SIZES=(512 1024 2048)

LOG_DIR="logs"
mkdir -p $LOG_DIR

# array con i numeri di thread/processi da testare
CORES=(1 2 4 8)
PROCS=(1 2 4 8)

for N in "${SIZES[@]}"; do
    MATRIX_FILE="data/matrix_${N}.txt"
    LOG_NAME="strong_scaling_results_${N}_finale.log"
    LOG_FILE="${LOG_DIR}/${LOG_NAME}"

    echo "==========================================" | tee $LOG_FILE
    echo " INIZIO TEST: STRONG SCALING" | tee -a $LOG_FILE
    echo " Matrice: $N x $N" | tee -a $LOG_FILE
    echo "==========================================" | tee -a $LOG_FILE

    if [ ! -f "$MATRIX_FILE" ]; then
        echo "[!] Errore: File $MATRIX_FILE non trovato. Generalo con lo script Python." | tee -a $LOG_FILE
        continue
    fi

    # TEST OPENMP
    for c in "${CORES[@]}"; do
        echo "##########################################" | tee -a $LOG_FILE
        echo ">>> ESECUZIONE CON $c CORE/THREAD <<<" | tee -a $LOG_FILE
        echo "##########################################" | tee -a $LOG_FILE

        export OMP_NUM_THREADS=$c
        echo -e "\n--- OpenMP Opt ($c thread) ---" | tee -a $LOG_FILE
        ./denoising_omp $N $MATRIX_FILE opt | tee -a $LOG_FILE

        echo "" | tee -a $LOG_FILE
    done

    # TEST MPI
    for p in "${PROCS[@]}"; do
        echo "##########################################" | tee -a $LOG_FILE
        echo ">>> ESECUZIONE CON $p PROCESSI MPI <<<" | tee -a $LOG_FILE
        echo "##########################################" | tee -a $LOG_FILE

        # fissiamo a 1 il numero di thread
        export OMP_NUM_THREADS=1 
        echo -e "\n--- MPI Puro ($p processi) ---" | tee -a $LOG_FILE
        mpirun -n $p ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE

        echo "" | tee -a $LOG_FILE
    done

    echo "##########################################" | tee -a $LOG_FILE
    echo ">>> TEST IBRIDO (Fisso: 8 Core) <<<" | tee -a $LOG_FILE
    echo "##########################################" | tee -a $LOG_FILE

    echo -e "\n--- Ibrido (2 Processi MPI x 4 Thread OpenMP) ---" | tee -a $LOG_FILE
    export OMP_NUM_THREADS=4
    mpirun -n 2 ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE
        
    echo -e "\n--- Ibrido (4 Processi MPI x 2 Thread OpenMP) ---" | tee -a $LOG_FILE
    export OMP_NUM_THREADS=2
    mpirun -n 4 ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE

    echo "" | tee -a $LOG_FILE
    echo "[*] Strong Scaling completato per la matrice $N x $N." | tee -a $LOG_FILE
done