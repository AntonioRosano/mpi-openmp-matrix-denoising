# HPC Image Denoising (MPI + OpenMP)

Questo progetto implementa e analizza le prestazioni di algoritmi numerici parallelizzati per il *denoising* di immagini astrofisiche (simulate tramite matrici con rumore gaussiano). Il codice è sviluppato in C utilizzando un approccio di programmazione ibrida: **MPI** per la memoria distribuita e **OpenMP** per la memoria condivisa.

## Obiettivi del Progetto
Il progetto effettua un benchmarking per valutare lo *speedup* e la scalabilità dei seguenti metodi:
1. **Metodo delle Potenze** (Parallelizzato ibrido)
2. **Algoritmo QR Iterativo** (Parallelizzato ibrido)
3. **SVD (Singular Value Decomposition)** (Tramite libreria esterna, utilizzata come *ground-truth* sequenziale per la misurazione dell'errore).

## Struttura della Repository
* `dataset_generator.py`: Script Python per la generazione di matrici sintetiche di test a basso rango con rumore gaussiano.
* `requirements.txt`: Elenco delle dipendenze Python necessarie per il generatore.
* `.gitignore`: Regole per escludere file binari, matrici pesanti e ambienti virtuali dalla repository.
* `/src`: (In sviluppo) Codice sorgente C/C++ degli algoritmi paralleli.
* `/docs`: (In sviluppo) Relazione finale e grafici di *strong* e *weak scaling*.

---

## ⚙️ Setup e Installazione

Per riprodurre l'ambiente di lavoro ed eseguire il codice, segui questi passaggi:

### 1. Prerequisiti di Sistema
Assicurati di avere installati sul tuo sistema:
* **Compilatore C/C++** con supporto a OpenMP (es. `gcc`).
* **Libreria MPI** (es. OpenMPI o MPICH).
* **Python 3.8+** (Solo per la generazione del dataset).

### 2. Setup dell'Ambiente Python (Per il Dataset)
Per evitare conflitti tra le librerie, è caldamente consigliato l'utilizzo di un ambiente virtuale.

```bash
# 1. Clona la repository ed entra nella cartella
git clone <URL-DELLA-TUA-REPO>
cd <NOME-CARTELLA-REPO>

# 2. Crea l'ambiente virtuale
python3 -m venv venv

# 3. Attiva l'ambiente virtuale
# Su Linux/macOS:
source venv/bin/activate
# Su Windows (Command Prompt):
venv\Scripts\activate.bat

# 4. Installa le dipendenze
pip install -r requirements.txt