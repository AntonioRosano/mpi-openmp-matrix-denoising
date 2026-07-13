# HPC Image Denoising (MPI + OpenMP)

Questo progetto implementa e analizza le prestazioni di algoritmi numerici parallelizzati per il *denoising* di immagini (simulate tramite matrici con rumore gaussiano). Il codice è sviluppato in C e valuta diverse tecniche di ottimizzazione e un approccio di programmazione ibrida: **MPI** per la memoria distribuita e **OpenMP** per la memoria condivisa.

## Obiettivi del Progetto
Il progetto effettua un benchmarking per valutare le ottimizzazioni della cache, lo *speedup* e la scalabilità (strong e weak scaling) dei seguenti metodi:
1. **Metodo delle Potenze** (per il calcolo dell'autovettore dominante)
2. **Algoritmo QR Iterativo** (per il calcolo di autovalori e autovettori)

I metodi sono stati implementati in diverse versioni:
- **Naive**: Implementazione base, non ottimizzata.
- **Opt**: Implementazione ottimizzata per l'uso della cache e parallelizzata in memoria condivisa tramite OpenMP.
- **Blocked**: Implementazione con loop-tiling/blocking per sfruttare al massimo la gerarchia di memoria (OpenMP).
- **Hybrid (MPI+OpenMP)**: Implementazione parallela ibrida per sistemi a memoria distribuita e condivisa.

## Struttura della Repository
- `data/`: Directory in cui verranno generate e salvate le matrici (con rumore) e i relativi *ground-truth*.
- `logs/`: Directory con i log delle esecuzioni prodotte durante i benchmark di scaling.
- `plots/`: Contiene i grafici (es. scaling, efficienza, andamento dei tempi) generati dagli script Python.
- `script/`: Script Python per la generazione del dataset (`dataset_generator.py`) e il plotting dei risultati (`plot_results.py`, ecc.), nonché script Bash per automatizzare i benchmark (es. `strong_scaling.sh`, `weak_scaling.sh`).
- `src/`: Codice sorgente C dei metodi implementati (`naive`, `opt`, `blocked`, `mpi`) e utility varie.
- `Makefile`: Script per la compilazione automatizzata dei file sorgenti.
- `Presentazione_finale_Antonio_Rosano.pdf`: Presentazione e relazione finale del progetto con l'analisi completa dei risultati e del codice.
- `requirements.txt`: Elenco delle dipendenze Python necessarie per l'ambiente virtuale.

---

## Setup e Installazione

Per preparare l'ambiente ed eseguire il codice, procedere come segue:

### 1. Prerequisiti di Sistema
Assicurarsi di avere:
* **Compilatore C/C++** con supporto a OpenMP (es. `gcc` o `clang` con `libomp`).
* **Libreria MPI** (es. OpenMPI o MPICH).
* **Python 3.8+** (per la generazione del dataset sintetico e per creare i grafici).

### 2. Setup dell'Ambiente Python
Per eseguire gli script Python presenti in `script/`, è caldamente consigliato configurare un ambiente virtuale.

```bash
# 1. Crea l'ambiente virtuale (nella cartella root del progetto)
python3 -m venv venv

# 2. Attiva l'ambiente virtuale
# Su Linux/macOS:
source venv/bin/activate
# Su Windows (Command Prompt):
venv\Scripts\activate.bat

# 3. Installa le dipendenze
pip install -r requirements.txt
```

### 3. Generazione del Dataset
Prima di procedere, è necessario generare il dataset di matrici che verrà posizionato in `data/`:
```bash
python3 script/dataset_generator.py
```

### 4. Compilazione
Il `Makefile` compila tutti gli eseguibili:
```bash
make all
```
Gli eseguibili generati saranno:
- `denoising_serial`
- `denoising_omp`
- `denoising_mpi`

(È possibile pulire la cartella di compilazione con il comando `make clean`).

### 5. Esecuzione
È possibile eseguire i file manualmente:
- **Seriale / OpenMP**: `./denoising_omp` (verificare i parametri del programma) e impostare i thread desiderati tramite `OMP_NUM_THREADS`.
- **MPI**: `mpirun -np <numero-processi> ./denoising_mpi`
- È inoltre possibile utilizzare gli script Bash forniti in `script/` (es. `./script/strong_scaling.sh`) per effettuare sessioni estese di benchmarking automatizzate.