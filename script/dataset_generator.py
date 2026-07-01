import numpy as np
import argparse
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "..")
DEFAULT_OUTDIR = os.path.join(PROJECT_DIR, "data")

def main():

    parser = argparse.ArgumentParser(description="Generatore dataset per benchmarking Denoising")
    parser.add_argument("N", type=int, help="Dimensione della matrice (es. 512, 1024)")
    parser.add_argument("--noise", type=float, default=0.5, help="Deviazione standard del rumore gaussiano")
    parser.add_argument("--outdir", type=str, default=DEFAULT_OUTDIR, help="Cartella di destinazione per i file generati")
    args = parser.parse_args()

    N = args.N
    sigma = args.noise
    outdir = args.outdir

    os.makedirs(outdir, exist_ok=True)

    print(f"[*] Inizio generazione matrice {N}x{N} con rumore (sigma={sigma})...")
    print(f"[*] Directory di output impostata su: '{outdir}/'")

    # seed fisso per benchmarking
    np.random.seed(42)

    # creiamo una matrice di rango 1 facendo il prodotto esterno di due vettori
    print("[*] Creazione del segnale di base a basso rango...")
    u = np.random.rand(N)
    v = np.random.rand(N)
    clean_matrix = np.outer(u, v)

    # aggiunta del rumore
    print("[*] Aggiunta del rumore gaussiano...")
    noise = np.random.normal(0, sigma, (N, N))
    noisy_matrix = clean_matrix + noise

    # esportazione
    filename = os.path.join(outdir, f"matrix_{N}.txt")
    print(f"[*] Salvataggio della matrice su {filename}...")
    np.savetxt(filename, noisy_matrix, fmt="%.6f", delimiter=" ")

    # calcolo SVD (Ground Truth)
    print("[*] Calcolo della SVD per il ground truth...")
    U, S, Vt = np.linalg.svd(noisy_matrix)

    # salviamo i valori singolari (S) in un file separato
    gt_filename = os.path.join(outdir, f"groundtruth_S_{N}.txt")
    print(f"[*] Salvataggio dei valori singolari su {gt_filename}...")
    np.savetxt(gt_filename, S, fmt="%.6f", delimiter=" ")

    print("[*] Generazione completata con successo")

if __name__ == "__main__":
    main()