#!/bin/bash

# Posizioniamoci sempre nella root del progetto, indipendentemente da dove viene lanciato lo script
cd "$(dirname "$0")/.." || exit 1

# ==========================================
# CONFIGURAZIONE LOCALE (Mac)
# ==========================================
export OMP_NUM_THREADS=8

MPI_PROCS=4

SIZES=(512 1024 2048 4096)

LOG_DIR="logs"
LOG_NAME="benchmark_mac_results_1.log"
LOG_FILE="${LOG_DIR}/${LOG_NAME}"

mkdir -p "$LOG_DIR"

echo "==========================================" | tee $LOG_FILE
echo " INIZIO BENCHMARK (MAC)" | tee -a $LOG_FILE
echo " Thread OpenMP: $OMP_NUM_THREADS" | tee -a $LOG_FILE
echo " Processi MPI: $MPI_PROCS" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# compilazione di tutti gli eseguibili
echo "[*] Compilazione in corso..." | tee -a $LOG_FILE
make clean
make all

echo "[*] Compilazione terminata." | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# ciclo di test per ogni dimensione della matrice
for N in "${SIZES[@]}"; do
    MATRIX_FILE="data/matrix_${N}.txt"
    echo "##########################################" | tee -a $LOG_FILE
    echo ">>> TEST MATRICE: $N x $N <<<" | tee -a $LOG_FILE
    echo "##########################################" | tee -a $LOG_FILE

    # --- eseguiamo Naive solo se N < 4096 ---
    if [ "$N" -lt 4096 ]; then
        echo -e "\n--- 1. Seriale Naive ---" | tee -a $LOG_FILE
        ./denoising_serial $N $MATRIX_FILE naive | tee -a $LOG_FILE
    else
        echo -e "\n--- 1. Seriale Naive saltata (troppo lenta per 4096) ---" | tee -a $LOG_FILE
    fi

    echo -e "\n--- 2. Seriale Opt ---" | tee -a $LOG_FILE
    ./denoising_serial $N $MATRIX_FILE opt | tee -a $LOG_FILE

    echo -e "\n--- 3. Seriale Opt Blocked ---" | tee -a $LOG_FILE
    ./denoising_serial $N $MATRIX_FILE blocked | tee -a $LOG_FILE

    echo -e "\n--- 4. OpenMP Opt ---" | tee -a $LOG_FILE
    ./denoising_omp $N $MATRIX_FILE opt | tee -a $LOG_FILE

    echo -e "\n--- 5. OpenMP Opt Blocked ---" | tee -a $LOG_FILE
    ./denoising_omp $N $MATRIX_FILE blocked | tee -a $LOG_FILE

    echo -e "\n--- 6. MPI (Ibrido MPI + OpenMP) ---" | tee -a $LOG_FILE

    mpirun -n $MPI_PROCS ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE

    echo "" | tee -a $LOG_FILE
done

echo "==========================================" | tee -a $LOG_FILE
echo " BENCHMARK CONCLUSO. Risultati salvati in $LOG_FILE" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE