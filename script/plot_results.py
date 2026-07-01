import os
import re
import sys
import matplotlib.pyplot as plt

# Calcola la directory in cui si trova il progetto (un livello sopra rispetto alla cartella script)
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "..")

def parse_strong_scaling_log(filename):
    data = {
        'OpenMP': {'cores': [], 'potenze': [], 'qr': [], 'totale': []},
        'MPI': {'procs': [], 'potenze': [], 'qr': [], 'totale': []},
        'Hybrid': {'config': [], 'potenze': [], 'qr': [], 'totale': []}
    }
    
    current_mode = None
    
    with open(filename, 'r') as f:
        for line in f:
            if "--- OpenMP Opt" in line:
                m = re.search(r'\((\d+) thread\)', line)
                if m:
                    data['OpenMP']['cores'].append(int(m.group(1)))
                    current_mode = 'OpenMP'
            elif "--- MPI Puro" in line:
                m = re.search(r'\((\d+) processi\)', line)
                if m:
                    data['MPI']['procs'].append(int(m.group(1)))
                    current_mode = 'MPI'
            elif "--- Ibrido" in line:
                m = re.search(r'\((.*)\)', line)
                if m:
                    data['Hybrid']['config'].append(m.group(1))
                    current_mode = 'Hybrid'
            
            elif "Tempo Metodo Potenze" in line and current_mode:
                m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                if m:
                    data[current_mode]['potenze'].append(float(m.group(1)))
            elif "Tempo Algoritmo QR" in line and current_mode:
                m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                if m:
                    data[current_mode]['qr'].append(float(m.group(1)))
            elif "TEMPO TOTALE" in line and current_mode:
                m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                if m:
                    data[current_mode]['totale'].append(float(m.group(1)))
                    current_mode = None 
                    
    return data

def plot_combined_time(data, matrix_size):
    plt.figure(figsize=(12, 8))
    
    if data['OpenMP']['cores']:
        x = data['OpenMP']['cores']
        if len(data['OpenMP']['qr']) > 0:
            plt.plot(x, data['OpenMP']['qr'], marker='v', linestyle='--', color='#1f77b4', linewidth=2, label='OpenMP - QR')
        if len(data['OpenMP']['potenze']) > 0:
            plt.plot(x, data['OpenMP']['potenze'], marker='^', linestyle=':', color='#1f77b4', linewidth=2, label='OpenMP - Potenze')

    if data['MPI']['procs']:
        x = data['MPI']['procs']
        if len(data['MPI']['qr']) > 0:
            plt.plot(x, data['MPI']['qr'], marker='v', linestyle='--', color='#ff7f0e', linewidth=2, label='MPI - QR')
        if len(data['MPI']['potenze']) > 0:
            plt.plot(x, data['MPI']['potenze'], marker='^', linestyle=':', color='#ff7f0e', linewidth=2, label='MPI - Potenze')

    if data['Hybrid']['config']:
        hybrid_colors = ['#2ca02c', '#d62728']
        hybrid_labels = ['Hybrid 2P x 4T', 'Hybrid 4P x 2T']
        for i, config in enumerate(data['Hybrid']['config']):
            c = hybrid_colors[i % len(hybrid_colors)]
            lab = hybrid_labels[i % len(hybrid_labels)]
            
            x_offset = -0.2 if i == 0 else 0.2
            x_pos = 8 + x_offset
            
            if len(data['Hybrid']['qr']) > i:
                plt.plot(x_pos, data['Hybrid']['qr'][i], marker='v', color=c, markersize=10, linestyle='None', label=f'{lab} - QR')
            if len(data['Hybrid']['potenze']) > i:
                plt.plot(x_pos, data['Hybrid']['potenze'][i], marker='^', color=c, markersize=10, linestyle='None', label=f'{lab} - Potenze')

    plt.xlabel('Numero di Core / Processi', fontsize=12)
    plt.ylabel('Tempo (secondi)', fontsize=12)
    plt.title(f'Strong Scaling - Tempi di Esecuzione (N={matrix_size})', fontsize=14)
    plt.xticks([1, 2, 4, 8])
    plt.yscale('log') 
    plt.grid(True, which="both", linestyle='--', alpha=0.7)
    
    plt.legend(fontsize=10, bbox_to_anchor=(1.05, 1), loc='upper left')
    
    # Salva in ../plots/
    out_dir = os.path.join(PROJECT_DIR, "plots")
    os.makedirs(out_dir, exist_ok=True)
    filename = os.path.join(out_dir, f'plot_combined_time_N{matrix_size}.png')
    
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"[+] Generato: {os.path.relpath(filename, SCRIPT_DIR)}")

def plot_combined_speedup(data, matrix_size):
    plt.figure(figsize=(12, 8))
    
    t1_omp_qr = data['OpenMP']['qr'][0] if len(data['OpenMP']['qr']) > 0 else 1
    t1_omp_pot = data['OpenMP']['potenze'][0] if len(data['OpenMP']['potenze']) > 0 else 1
    
    t1_mpi_qr = data['MPI']['qr'][0] if len(data['MPI']['qr']) > 0 else 1
    t1_mpi_pot = data['MPI']['potenze'][0] if len(data['MPI']['potenze']) > 0 else 1

    if data['OpenMP']['cores']:
        x = data['OpenMP']['cores']
        if len(data['OpenMP']['qr']) > 0:
            su = [t1_omp_qr / t for t in data['OpenMP']['qr']]
            plt.plot(x, su, marker='v', linestyle='--', color='#1f77b4', linewidth=2, label='OpenMP - QR')
        if len(data['OpenMP']['potenze']) > 0:
            su = [t1_omp_pot / t for t in data['OpenMP']['potenze']]
            plt.plot(x, su, marker='^', linestyle=':', color='#1f77b4', linewidth=2, label='OpenMP - Potenze')

    if data['MPI']['procs']:
        x = data['MPI']['procs']
        if len(data['MPI']['qr']) > 0:
            su = [t1_mpi_qr / t for t in data['MPI']['qr']]
            plt.plot(x, su, marker='v', linestyle='--', color='#ff7f0e', linewidth=2, label='MPI - QR')
        if len(data['MPI']['potenze']) > 0:
            su = [t1_mpi_pot / t for t in data['MPI']['potenze']]
            plt.plot(x, su, marker='^', linestyle=':', color='#ff7f0e', linewidth=2, label='MPI - Potenze')

    if data['Hybrid']['config']:
        hybrid_colors = ['#2ca02c', '#d62728']
        hybrid_labels = ['Hybrid 2P x 4T', 'Hybrid 4P x 2T']
        for i, config in enumerate(data['Hybrid']['config']):
            c = hybrid_colors[i % len(hybrid_colors)]
            lab = hybrid_labels[i % len(hybrid_labels)]
            
            x_offset = -0.2 if i == 0 else 0.2
            x_pos = 8 + x_offset
            
            if len(data['Hybrid']['qr']) > i:
                su_qr = t1_mpi_qr / data['Hybrid']['qr'][i]
                plt.plot(x_pos, su_qr, marker='v', color=c, markersize=10, linestyle='None', label=f'{lab} - QR')
            if len(data['Hybrid']['potenze']) > i:
                su_pot = t1_mpi_pot / data['Hybrid']['potenze'][i]
                plt.plot(x_pos, su_pot, marker='^', color=c, markersize=10, linestyle='None', label=f'{lab} - Potenze')

    plt.plot([1, 8], [1, 8], 'k--', label='Ideale', alpha=0.6)
    
    plt.xlabel('Numero di Core / Processi', fontsize=12)
    plt.ylabel('Speedup', fontsize=12)
    plt.title(f'Strong Scaling - Speedup (N={matrix_size})', fontsize=14)
    plt.xticks([1, 2, 4, 8])
    plt.yticks(range(1, 9))
    plt.grid(True, linestyle='--', alpha=0.7)
    
    plt.legend(fontsize=10, bbox_to_anchor=(1.05, 1), loc='upper left')
    
    # Salva in ../plots/
    out_dir = os.path.join(PROJECT_DIR, "plots")
    os.makedirs(out_dir, exist_ok=True)
    filename = os.path.join(out_dir, f'plot_combined_speedup_N{matrix_size}.png')
    
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"[+] Generato: {os.path.relpath(filename, SCRIPT_DIR)}")

if __name__ == "__main__":
    # Path di default verso ../logs/
    target_log = os.path.join(PROJECT_DIR, "logs", "strong_scaling_results_512_finale.log")
    
    if len(sys.argv) > 1:
        arg_path = sys.argv[1]
        if os.path.exists(arg_path):
            target_log = arg_path
        else:
            target_log = os.path.join(PROJECT_DIR, arg_path)
            
    match = re.search(r'_(\d+)(_|\.)', target_log)
    if match:
        matrix_dim = match.group(1)
    else:
        fallback = re.search(r'(\d+)', target_log)
        matrix_dim = fallback.group(1) if fallback else "UNKNOWN"
        
    if os.path.exists(target_log):
        print(f"[*] Parsing del file {os.path.relpath(target_log, SCRIPT_DIR)} (N={matrix_dim})...")
        parsed_data = parse_strong_scaling_log(target_log)
        
        plot_combined_time(parsed_data, matrix_dim)
        plot_combined_speedup(parsed_data, matrix_dim)
        
        print(f"[*] I grafici per N={matrix_dim} sono stati aggiornati con successo.")
    else:
        print(f"[!] Errore: File log '{os.path.relpath(target_log, SCRIPT_DIR)}' non trovato.")
