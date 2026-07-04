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

def plot_weak_combined(data, model_name):
    if model_name == 'OpenMP' and not data['OpenMP']['cores']: return
    if model_name == 'MPI' and not data['MPI']['procs']: return
    if model_name == 'Hybrid' and not data['Hybrid']['config']: return

    # Crea una figura con 2 subplot affiancati
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # Asse secondario per il TEMPO (Sempre attivo per separare le scale di QR e Potenze)
    ax1_pot = ax1.twinx()
    
    color_qr = '#e74c3c'      # Rosso
    color_pot = '#f39c12'     # Arancione
    
    if model_name == 'OpenMP':
        x = data['OpenMP']['cores']
        qr_vals = data['OpenMP']['qr']
        pot_vals = data['OpenMP']['potenze']
        
        if qr_vals:
            ax1.plot(x, qr_vals, marker='v', linestyle='-', color=color_qr, linewidth=2, label='QR')
            t1_qr = qr_vals[0]
            eff_qr = [t1_qr / t for t in qr_vals]
            ax2.plot(x, eff_qr, marker='v', linestyle='-', color=color_qr, linewidth=2, label='QR')
            
        if pot_vals:
            ax1_pot.plot(x, pot_vals, marker='^', linestyle='--', color=color_pot, linewidth=2, label='Potenze')
            t1_pot = pot_vals[0]
            eff_pot = [t1_pot / t for t in pot_vals]
            ax2.plot(x, eff_pot, marker='^', linestyle='--', color=color_pot, linewidth=2, label='Potenze')
            
        ax1.set_xlabel('Numero di Thread (N cresce con P)', fontsize=12)
        ax1.set_xticks(x)
        ax2.set_xlabel('Numero di Thread (N cresce con P)', fontsize=12)
        ax2.set_xticks(x)
        ax2.plot([min(x), max(x)], [1, 1], 'k--', label='Ideale', alpha=0.6)
        
    elif model_name == 'MPI':
        x = data['MPI']['procs']
        qr_vals = data['MPI']['qr']
        pot_vals = data['MPI']['potenze']
        
        if qr_vals:
            ax1.plot(x, qr_vals, marker='v', linestyle='-', color=color_qr, linewidth=2, label='QR')
            t1_qr = qr_vals[0]
            eff_qr = [t1_qr / t for t in qr_vals]
            ax2.plot(x, eff_qr, marker='v', linestyle='-', color=color_qr, linewidth=2, label='QR')
            
        if pot_vals:
            ax1_pot.plot(x, pot_vals, marker='^', linestyle='--', color=color_pot, linewidth=2, label='Potenze')
            t1_pot = pot_vals[0]
            eff_pot = [t1_pot / t for t in pot_vals]
            ax2.plot(x, eff_pot, marker='^', linestyle='--', color=color_pot, linewidth=2, label='Potenze')
            
        ax1.set_xlabel('Numero di Processi MPI (N cresce con P)', fontsize=12)
        ax1.set_xticks(x)
        ax2.set_xlabel('Numero di Processi MPI (N cresce con P)', fontsize=12)
        ax2.set_xticks(x)
        ax2.plot([min(x), max(x)], [1, 1], 'k--', label='Ideale', alpha=0.6)
        
    elif model_name == 'Hybrid':
        x_labels = data['Hybrid']['config']
        x = range(len(x_labels))
        qr_vals = data['Hybrid']['qr']
        pot_vals = data['Hybrid']['potenze']
        
        color_qr_h = color_qr
        color_pot_h = color_pot
        
        ax2_pot = ax2.twinx()
        
        if qr_vals:
            offsets_qr = [pos - 0.2 for pos in x]
            ax1.bar(offsets_qr, qr_vals, width=0.4, color=color_qr_h, alpha=0.8, label='QR')
            
            t1_qr = data['MPI']['qr'][0] if len(data['MPI']['qr']) > 0 else 1
            eff_qr = [t1_qr / t for t in qr_vals]
            ax2.bar(offsets_qr, eff_qr, width=0.4, color=color_qr_h, alpha=0.8, label='QR')
            
        if pot_vals:
            offsets_pot = [pos + 0.2 for pos in x]
            ax1_pot.bar(offsets_pot, pot_vals, width=0.4, color=color_pot_h, alpha=0.8, label='Potenze')
            
            t1_pot = data['MPI']['potenze'][0] if len(data['MPI']['potenze']) > 0 else 1
            eff_pot = [t1_pot / t for t in pot_vals]
            ax2_pot.bar(offsets_pot, eff_pot, width=0.4, color=color_pot_h, alpha=0.8, label='Potenze')
            
        ax1.set_xlabel('Configurazione Ibrida (N cresce con P)', fontsize=12)
        ax1.set_xticks(x)
        ax1.set_xticklabels(x_labels)
        ax2.set_xlabel('Configurazione Ibrida (N cresce con P)', fontsize=12)
        ax2.set_xticks(x)
        ax2.set_xticklabels(x_labels)

    # ==========================
    # Stili asse 1 (Tempo - QR)
    # ==========================
    ax1.set_ylabel('Tempo QR (secondi)', fontsize=12, color=color_qr, fontweight='bold')
    ax1.tick_params(axis='y', labelcolor=color_qr)
    ax1.set_title('Tempo di Esecuzione', fontsize=14)
    ax1.grid(True, axis='y', linestyle='--', alpha=0.3)
    ax1.set_yscale('linear')
    
    # ===============================
    # Stili asse 1_pot (Tempo - Potenze)
    # ===============================
    ax1_pot.set_ylabel('Tempo Potenze (secondi)', fontsize=12, color=color_pot, fontweight='bold')
    ax1_pot.tick_params(axis='y', labelcolor=color_pot)
    ax1_pot.set_yscale('linear')
    
    lines_1, labels_1 = ax1.get_legend_handles_labels()
    lines_2, labels_2 = ax1_pot.get_legend_handles_labels()
    ax1.legend(lines_1 + lines_2, labels_1 + labels_2, fontsize=10, loc='upper center')
    
    # ==========================
    # Stili asse 2 (Efficienza)
    # ==========================
    if model_name in ['OpenMP', 'MPI']:
        # Singolo asse per OpenMP e MPI
        ax2.set_ylabel('Efficienza Weak (T1 / Tp)', fontsize=12)
        ax2.set_title('Efficienza', fontsize=14)
        ax2.grid(True, axis='y', linestyle='--', alpha=0.7)
        ax2.legend(fontsize=10)
        ax2.set_yscale('linear')
    else:
        # Doppio asse per l'Ibrido
        ax2.set_ylabel('Efficienza QR (T1 / Tp)', fontsize=12, color=color_qr, fontweight='bold')
        ax2.tick_params(axis='y', labelcolor=color_qr)
        ax2.set_title('Efficienza Weak', fontsize=14)
        ax2.grid(True, axis='y', linestyle='--', alpha=0.3)
        ax2.set_yscale('linear')
        
        ax2_pot.set_ylabel('Efficienza Potenze (T1 / Tp)', fontsize=12, color=color_pot, fontweight='bold')
        ax2_pot.tick_params(axis='y', labelcolor=color_pot)
        ax2_pot.set_yscale('linear')
        
        lines_3, labels_3 = ax2.get_legend_handles_labels()
        lines_4, labels_4 = ax2_pot.get_legend_handles_labels()
        ax2.legend(lines_3 + lines_4, labels_3 + labels_4, fontsize=10, loc='upper center')
    
    # Titolo Generale
    plt.suptitle(f"Weak Scaling: {model_name}", fontsize=16, y=1.05)
    
    out_dir = os.path.join(PROJECT_DIR, "plots")
    os.makedirs(out_dir, exist_ok=True)
    filename = os.path.join(out_dir, f'plot_weak_{model_name.lower()}.png')
    
    plt.tight_layout()
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
        
        models = ['OpenMP', 'MPI', 'Hybrid']
        
        for model in models:
            plot_weak_combined(parsed_data, model)
                
        print("[*] I grafici combinati del weak scaling sono stati generati con successo.")
    else:
        print(f"[!] Errore: File log '{os.path.relpath(target_log, SCRIPT_DIR)}' non trovato.")
