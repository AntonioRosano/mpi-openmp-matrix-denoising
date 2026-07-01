import os
import re
import sys
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "..")

def parse_weak_scaling_log(filename):
    data = {
        'OpenMP': {'cores': [], 'potenze': [], 'qr': []},
        'MPI': {'procs': [], 'potenze': [], 'qr': []},
        'Hybrid': {'config': [], 'cores': [], 'potenze': [], 'qr': []}
    }
    
    current_mode = None
    current_cores = 1
    
    with open(filename, 'r') as f:
        for line in f:
            if ">>> TEST:" in line:
                m = re.search(r'TEST:\s*(\d+)\s*Processi', line)
                if m:
                    current_cores = int(m.group(1))
            
            elif "--- OpenMP Opt" in line:
                data['OpenMP']['cores'].append(current_cores)
                current_mode = 'OpenMP'
            elif "--- MPI Puro" in line:
                data['MPI']['procs'].append(current_cores)
                current_mode = 'MPI'
            elif "--- Ibrido" in line:
                m = re.search(r'\((.*)\)', line)
                if m:
                    data['Hybrid']['config'].append(m.group(1))
                    data['Hybrid']['cores'].append(current_cores)
                    current_mode = 'Hybrid'
            
            elif "Tempo Metodo Potenze" in line and current_mode:
                m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                if m:
                    data[current_mode]['potenze'].append(float(m.group(1)))
            elif "Tempo Algoritmo QR" in line and current_mode:
                m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                if m:
                    data[current_mode]['qr'].append(float(m.group(1)))
                    current_mode = None 
                    
    return data

def plot_weak_time(data):
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
        hybrid_colors = ['#2ca02c', '#d62728', '#9467bd']
        for i, config in enumerate(data['Hybrid']['config']):
            c = hybrid_colors[i % len(hybrid_colors)]
            cores = data['Hybrid']['cores'][i]
            lab = f'Hybrid ({config})'
            
            x_offset = -0.15 if i % 2 == 0 else 0.15
            x_pos = cores + x_offset
            
            if len(data['Hybrid']['qr']) > i:
                plt.plot(x_pos, data['Hybrid']['qr'][i], marker='v', color=c, markersize=10, linestyle='None', label=f'{lab} - QR')
            if len(data['Hybrid']['potenze']) > i:
                plt.plot(x_pos, data['Hybrid']['potenze'][i], marker='^', color=c, markersize=10, linestyle='None', label=f'{lab} - Potenze')

    plt.xlabel('Numero di Core / Processi', fontsize=12)
    plt.ylabel('Tempo (secondi)', fontsize=12)
    plt.title('Weak Scaling - Tempi di Esecuzione', fontsize=14)
    plt.xticks([1, 2, 4, 8])
    plt.yscale('log')
    plt.grid(True, which="both", linestyle='--', alpha=0.7)
    
    plt.legend(fontsize=10, bbox_to_anchor=(1.05, 1), loc='upper left')
    
    out_dir = os.path.join(PROJECT_DIR, "plots")
    os.makedirs(out_dir, exist_ok=True)
    filename = os.path.join(out_dir, 'plot_weak_time.png')
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"[+] Generato: {os.path.relpath(filename, SCRIPT_DIR)}")

def plot_weak_efficiency(data):
    plt.figure(figsize=(12, 8))
    
    # E = T(1) / T(p)
    t1_omp_qr = data['OpenMP']['qr'][0] if len(data['OpenMP']['qr']) > 0 else 1
    t1_omp_pot = data['OpenMP']['potenze'][0] if len(data['OpenMP']['potenze']) > 0 else 1
    
    t1_mpi_qr = data['MPI']['qr'][0] if len(data['MPI']['qr']) > 0 else 1
    t1_mpi_pot = data['MPI']['potenze'][0] if len(data['MPI']['potenze']) > 0 else 1

    if data['OpenMP']['cores']:
        x = data['OpenMP']['cores']
        if len(data['OpenMP']['qr']) > 0:
            eff = [t1_omp_qr / t for t in data['OpenMP']['qr']]
            plt.plot(x, eff, marker='v', linestyle='--', color='#1f77b4', linewidth=2, label='OpenMP - QR')
        if len(data['OpenMP']['potenze']) > 0:
            eff = [t1_omp_pot / t for t in data['OpenMP']['potenze']]
            plt.plot(x, eff, marker='^', linestyle=':', color='#1f77b4', linewidth=2, label='OpenMP - Potenze')

    if data['MPI']['procs']:
        x = data['MPI']['procs']
        if len(data['MPI']['qr']) > 0:
            eff = [t1_mpi_qr / t for t in data['MPI']['qr']]
            plt.plot(x, eff, marker='v', linestyle='--', color='#ff7f0e', linewidth=2, label='MPI - QR')
        if len(data['MPI']['potenze']) > 0:
            eff = [t1_mpi_pot / t for t in data['MPI']['potenze']]
            plt.plot(x, eff, marker='^', linestyle=':', color='#ff7f0e', linewidth=2, label='MPI - Potenze')

    if data['Hybrid']['config']:
        hybrid_colors = ['#2ca02c', '#d62728', '#9467bd']
        for i, config in enumerate(data['Hybrid']['config']):
            c = hybrid_colors[i % len(hybrid_colors)]
            cores = data['Hybrid']['cores'][i]
            lab = f'Hybrid ({config})'
            
            x_offset = -0.15 if i % 2 == 0 else 0.15
            x_pos = cores + x_offset
            
            if len(data['Hybrid']['qr']) > i:
                eff_qr = t1_mpi_qr / data['Hybrid']['qr'][i]
                plt.plot(x_pos, eff_qr, marker='v', color=c, markersize=10, linestyle='None', label=f'{lab} - QR')
            if len(data['Hybrid']['potenze']) > i:
                eff_pot = t1_mpi_pot / data['Hybrid']['potenze'][i]
                plt.plot(x_pos, eff_pot, marker='^', color=c, markersize=10, linestyle='None', label=f'{lab} - Potenze')

    # Linea Efficienza Ideale (costante a 1.0)
    plt.plot([1, 8], [1, 1], 'k--', label='Ideale', alpha=0.6)
    
    plt.xlabel('Numero di Core / Processi', fontsize=12)
    plt.ylabel('Efficienza (T1 / Tp)', fontsize=12)
    plt.title('Weak Scaling - Efficienza', fontsize=14)
    plt.xticks([1, 2, 4, 8])
    plt.grid(True, linestyle='--', alpha=0.7)
    
    plt.legend(fontsize=10, bbox_to_anchor=(1.05, 1), loc='upper left')
    
    out_dir = os.path.join(PROJECT_DIR, "plots")
    os.makedirs(out_dir, exist_ok=True)
    filename = os.path.join(out_dir, 'plot_weak_efficiency.png')
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"[+] Generato: {os.path.relpath(filename, SCRIPT_DIR)}")

if __name__ == "__main__":
    target_log = os.path.join(PROJECT_DIR, "logs", "weak_scaling_result_finale.log")
    
    if len(sys.argv) > 1:
        arg_path = sys.argv[1]
        if os.path.exists(arg_path):
            target_log = arg_path
        else:
            target_log = os.path.join(PROJECT_DIR, arg_path)
            
    if os.path.exists(target_log):
        print(f"[*] Parsing del file {os.path.relpath(target_log, SCRIPT_DIR)}...")
        parsed_data = parse_weak_scaling_log(target_log)
        
        plot_weak_time(parsed_data)
        plot_weak_efficiency(parsed_data)
        
        print(f"[*] I grafici del weak scaling sono stati generati con successo in plots/.")
    else:
        print(f"[!] Errore: File log '{os.path.relpath(target_log, SCRIPT_DIR)}' non trovato.")
