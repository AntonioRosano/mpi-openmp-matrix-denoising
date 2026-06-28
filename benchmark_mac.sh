#!/bin/bash

# ==========================================
# SETUP AMBIENTE HPC PER APPLE SILICON
# ==========================================
# Costringiamo OpenMP a usare solo i P-Cores (modifica il 4 se hai un processore Max/Pro/Ultra)
export OMP_NUM_THREADS=4 
# Diciamo ai P-Cores di dormire alle barriere invece di intasare il kernel col busy-waiting
export OMP_WAIT_POLICY=passive 
# Disabilitiamo esplicitamente l'Affinity per evitare i warning "not supported" su macOS
export OMP_PROC_BIND=false 

# Dimensioni delle matrici da testare
SIZES=(128 512 1024 2048)

LOG_FILE="benchmark_results.log"

# Pulisce lo schermo e il vecchio log
clear
echo "==========================================" | tee $LOG_FILE
echo " INIZIO BENCHMARK HPC MATRICI" | tee -a $LOG_FILE
echo " Data: $(date)" | tee -a $LOG_FILE
echo " Thread OMP: $OMP_NUM_THREADS | Policy: $OMP_WAIT_POLICY" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# Ricompila i binari in modo pulito
echo "[*] Compilazione dei binari (Seriale e OpenMP)..."
make clean > /dev/null
make > /dev/null
if [ $? -ne 0 ]; then
    echo "[!] Errore di compilazione. Interruzione."
    exit 1
fi
echo "[*] Compilazione completata con successo." | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# Loop principale sulle dimensioni
for N in "${SIZES[@]}"; do
    MATRIX_FILE="data/matrix_${N}.txt"
    
    echo "------------------------------------------" | tee -a $LOG_FILE
    echo ">>> TEST MATRICE: $N x $N <<<" | tee -a $LOG_FILE
    echo "------------------------------------------" | tee -a $LOG_FILE

    # NOTA: Assicurati che il file della matrice esista per N=128, 512, 1024, 2048
    # Se il tuo codice C lo genera in automatico in caso non esista, puoi ignorare questo commento.
    if [ ! -f "$MATRIX_FILE" ]; then
        echo "[!] Attenzione: Il file $MATRIX_FILE non esiste. (Potrebbe fallire se non viene generato in automatico)" | tee -a $LOG_FILE
    fi

    # 1. Test Naive (sul binario seriale per avere baseline pulita)
    echo -e "\n[ ESECUZIONE: NAIVE (Seriale non ottimizzato) ]" | tee -a $LOG_FILE
    ./denoising_serial $N $MATRIX_FILE naive | tee -a $LOG_FILE

    # 2. Test Optimized Seriale (Loop swapping e cache friendly)
    echo -e "\n[ ESECUZIONE: OPT SERIALE (Cache Friendly, 1 Core) ]" | tee -a $LOG_FILE
    ./denoising_serial $N $MATRIX_FILE opt | tee -a $LOG_FILE

    # 3. Test Optimized Parallelo (OpenMP su P-Cores)
    echo -e "\n[ ESECUZIONE: OPT OPENMP (HPC Parallelo, $OMP_NUM_THREADS Cores) ]" | tee -a $LOG_FILE
    ./denoising_omp $N $MATRIX_FILE opt | tee -a $LOG_FILE

    echo "" | tee -a $LOG_FILE
done

echo "==========================================" | tee -a $LOG_FILE
echo " BENCHMARK COMPLETATO! Risultati salvati in $LOG_FILE" | tee -a $LOG_FILE