#!/bin/bash

# Posizioniamoci sempre nella root del progetto, indipendentemente da dove viene lanciato lo script
cd "$(dirname "$0")/.." || exit 1

# Dimensioni calcolate per mantenere il carico cubico O(N^3) costante
# N1 = 512  (1 Processo)
# N2 = 645  (2 Processi)  -> 512 * cuberoot(2)
# N3 = 812  (4 Processi)  -> 512 * cuberoot(4)
# N4 = 1024 (8 Processi)  -> 512 * cuberoot(8)

SIZES=(512 646 812 1024)

PROCS=(1 2 4 8)

LOG_DIR="logs"
LOG_NAME="weak_scaling_result_finale.log"
LOG_FILE="${LOG_DIR}/${LOG_NAME}"

echo "==========================================" | tee $LOG_FILE
echo " INIZIO TEST: WEAK SCALING (O(N^3))" | tee -a $LOG_FILE
echo " Lavoro per core mantenuto costante." | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

for i in "${!PROCS[@]}"; do
    c=${PROCS[$i]}
    N=${SIZES[$i]}
    MATRIX_FILE="data/matrix_${N}.txt"

    echo "##########################################" | tee -a $LOG_FILE
    echo ">>> TEST: $c Processi | Matrice: $N x $N <<<" | tee -a $LOG_FILE
    echo "##########################################" | tee -a $LOG_FILE

    if [ ! -f "$MATRIX_FILE" ]; then
        echo "[!] Errore: File $MATRIX_FILE mancante." | tee -a $LOG_FILE
        echo "    Generalo con: python dataset_generator.py $N" | tee -a $LOG_FILE
        continue
    fi

    # 1. Test OpenMP
    export OMP_NUM_THREADS=$c
    echo -e "\n--- OpenMP Opt ($c thread) ---" | tee -a $LOG_FILE
    ./denoising_omp $N $MATRIX_FILE opt | tee -a $LOG_FILE

    # 2. Test MPI
    export OMP_NUM_THREADS=1 
    echo -e "\n--- MPI Puro ($c processi) ---" | tee -a $LOG_FILE
    mpirun -n $c ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE

    # 3. Test Ibrido (solo dove i core totali sono >= 4)
    if [ "$c" -eq 4 ]; then
        export OMP_NUM_THREADS=2
        echo -e "\n--- Ibrido (2 processi x 2 thread) ---" | tee -a $LOG_FILE
        mpirun -n 2 ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE
    elif [ "$c" -eq 8 ]; then
        export OMP_NUM_THREADS=4
        echo -e "\n--- Ibrido (2 processi x 4 thread) ---" | tee -a $LOG_FILE
        mpirun -n 2 ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE
        
        export OMP_NUM_THREADS=2
        echo -e "\n--- Ibrido (4 processi x 2 thread) ---" | tee -a $LOG_FILE
        mpirun -n 4 ./denoising_mpi $N $MATRIX_FILE | tee -a $LOG_FILE
    fi

    echo "" | tee -a $LOG_FILE
done

echo "[*] Weak Scaling completato." | tee -a $LOG_FILE