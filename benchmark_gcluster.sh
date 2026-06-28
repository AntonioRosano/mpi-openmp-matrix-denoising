#!/bin/bash
#SBATCH --job-name=matrix_hpc
#SBATCH --account=dl-course-q2
#SBATCH --partition=dl-course-q2
#SBATCH --qos=gpu-xlarge
#SBATCH --mem=16G
#SBATCH --cpus-per-task=8
#SBATCH --time=01:30:00
#SBATCH --output=job-%j.out

# 1. carico il compilatore standard del cluster
# module load gcc/12.2.0

# 2. setup dell'affinità dei thread OpenMP per Linux
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close
export OMP_PLACES=cores
export OMP_WAIT_POLICY=passive

# 3. variabili del benchmark
SIZES=(128 512 1024 2048)
LOG_FILE="benchmark_cluster_results.log"


echo "==========================================" | tee $LOG_FILE
echo " INIZIO BENCHMARK HPC SU CLUSTER DMI" | tee -a $LOG_FILE
echo " Data: $(date)" | tee -a $LOG_FILE
echo " Nodo Assegnato: $SLURMD_NODENAME" | tee -a $LOG_FILE
echo " Thread OMP: $OMP_NUM_THREADS" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 4. compilazione sul Compute Node
echo "[*] Compilazione dei binari in corso..." | tee -a $LOG_FILE
make clean > /dev/null
make > /dev/null
if [ $? -ne 0 ]; then
    echo "[!] ERRORE DI COMPILAZIONE. Interruzione." | tee -a $LOG_FILE
    exit 1
fi
echo "[*] Compilazione completata con successo." | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# 5. loop di esecuzione
for N in "${SIZES[@]}"; do
    MATRIX_FILE="data/matrix_${N}.txt"
    
    echo "------------------------------------------" | tee -a $LOG_FILE
    echo ">>> TEST MATRICE: $N x $N <<<" | tee -a $LOG_FILE
    echo "------------------------------------------" | tee -a $LOG_FILE

    # --- TEST NAIVE ---
    echo -e "\n[ ESECUZIONE: NAIVE (Seriale non ottimizzato) ]" | tee -a $LOG_FILE
    if [ "$N" -ge 2048 ]; then
        echo "   -> SALTATA: Per non sprecare preziose ore-calcolo del cluster per colpa del Cache Thrashing." | tee -a $LOG_FILE
    else
        ./denoising_serial $N $MATRIX_FILE naive | tee -a $LOG_FILE
    fi

    # --- TEST OPT SERIALE ---
    echo -e "\n[ ESECUZIONE: OPT SERIALE (Cache Friendly, 1 Core) ]" | tee -a $LOG_FILE
    ./denoising_serial $N $MATRIX_FILE opt | tee -a $LOG_FILE

    # --- TEST OPT OPENMP ---
    echo -e "\n[ ESECUZIONE: OPT OPENMP (HPC Parallelo, $OMP_NUM_THREADS Cores) ]" | tee -a $LOG_FILE
    ./denoising_omp $N $MATRIX_FILE opt | tee -a $LOG_FILE

    # --- TEST OPT OPENMP con tiling ---
    echo -e "\n[ ESECUZIONE: BLOCKED OPENMP (HPC Tiling, $OMP_NUM_THREADS Cores) ]" | tee -a $LOG_FILE
    ./denoising_omp $N $MATRIX_FILE blocked | tee -a $LOG_FILE

    echo "" | tee -a $LOG_FILE
done

echo "==========================================" | tee -a $LOG_FILE
echo " BENCHMARK COMPLETATO CON SUCCESSO!" | tee -a $LOG_FILE