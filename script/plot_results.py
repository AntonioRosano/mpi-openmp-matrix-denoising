import os
import re
import sys
import matplotlib.pyplot as plt

# Calcola la directory in cui si trova il progetto
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "..")

def parse_multiple_logs(log_files):
    data = {
        'OpenMP': {'cores': [], 'qr': {}, 'potenze': {}},
        'MPI': {'procs': [], 'qr': {}, 'potenze': {}},
        'Hybrid': {'config': [], 'qr': {}, 'potenze': {}}
    }
    
    for filename in log_files:
        match = re.search(r'(\d+)', os.path.basename(filename))
        if not match: continue
        N = int(match.group(1))
        
        data['OpenMP']['qr'][N] = []
        data['OpenMP']['potenze'][N] = []
        data['MPI']['qr'][N] = []
        data['MPI']['potenze'][N] = []
        data['Hybrid']['qr'][N] = []
        data['Hybrid']['potenze'][N] = []
        
        current_mode = None
        
        with open(filename, 'r') as f:
            for line in f:
                if "--- OpenMP Opt" in line:
                    m = re.search(r'\(([\d]+)\s+thread\)', line)
                    if m:
                        c = int(m.group(1))
                        if c not in data['OpenMP']['cores']: 
                            data['OpenMP']['cores'].append(c)
                    current_mode = 'OpenMP'
                elif "--- MPI Puro" in line:
                    m = re.search(r'\(([\d]+)\s+processi\)', line)
                    if m:
                        p = int(m.group(1))
                        if p not in data['MPI']['procs']: 
                            data['MPI']['procs'].append(p)
                    current_mode = 'MPI'
                elif "--- Ibrido" in line:
                    m = re.search(r'\((.*)\)', line)
                    if m:
                        conf = m.group(1)
                        if conf not in data['Hybrid']['config']: 
                            data['Hybrid']['config'].append(conf)
                    current_mode = 'Hybrid'
                
                elif "Tempo Metodo Potenze" in line and current_mode:
                    m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                    if m:
                        data[current_mode]['potenze'][N].append(float(m.group(1)))
                elif "Tempo Algoritmo QR" in line and current_mode:
                    m = re.search(r':\s*([\d\.]+)\s*secondi', line)
                    if m:
                        data[current_mode]['qr'][N].append(float(m.group(1)))
                elif "TEMPO TOTALE" in line and current_mode:
                    current_mode = None
                    
    return data

def plot_metric(data, model_name, method, metric='time'):
    sizes = sorted(data[model_name][method].keys())
    if not sizes: return
    
    plt.figure(figsize=(8, 6))
    
    # Palette colori per differenziare le matrici (Rosso, Arancione e Viola scuro)
    colors = ['#e74c3c', '#f39c12', '#8e44ad']
    
    if model_name in ['OpenMP', 'MPI']:
        x_key = 'cores' if model_name == 'OpenMP' else 'procs'
        x = data[model_name][x_key]
        if not x: return
        
        for i, N in enumerate(sizes):
            y_vals = data[model_name][method][N]
            if not y_vals or len(y_vals) != len(x): continue
            
            if metric == 'speedup':
                t1 = y_vals[0]
                y_vals = [t1 / t for t in y_vals]
                
            plt.plot(x, y_vals, marker='o', linewidth=2, color=colors[i%len(colors)], label=f'N={N}')
            
        if metric == 'speedup':
            plt.plot([1, max(x)], [1, max(x)], 'k--', label='Ideale', alpha=0.6)
            
        plt.xlabel('Numero di Thread' if model_name == 'OpenMP' else 'Numero di Processi MPI', fontsize=12)
        plt.xticks(x)
        
    elif model_name == 'Hybrid':
        x_labels = data['Hybrid']['config']
        if not x_labels: return
        x = range(len(x_labels))
        
        width = 0.25
        for i, N in enumerate(sizes):
            y_vals = data[model_name][method][N]
            if not y_vals or len(y_vals) != len(x): continue
            
            if metric == 'speedup':
                # Calcolo speedup usando il T1 di MPI puro su 1 nodo, se disponibile
                t1 = data['MPI'][method][N][0] if (N in data['MPI'][method] and len(data['MPI'][method][N]) > 0) else 1
                y_vals = [t1 / t for t in y_vals]
                
            offsets = [pos + (i - len(sizes)/2.0 + 0.5) * width for pos in x]
            plt.bar(offsets, y_vals, width=width, color=colors[i%len(colors)], alpha=0.8, label=f'N={N}')
            
        plt.xlabel('Configurazione Ibrida', fontsize=12)
        plt.xticks(x, x_labels)

    method_name = "Algoritmo QR" if method == 'qr' else "Metodo delle Potenze"
    y_label = 'Tempo (secondi)' if metric == 'time' else 'Speedup'
    title = f"{model_name} - {y_label} ({method_name})"
    
    plt.ylabel(y_label, fontsize=12)
    plt.title(title, fontsize=14)
    # Rimuoviamo la scala logaritmica come richiesto
    plt.yscale('linear')
    plt.grid(True, axis='y', linestyle='--', alpha=0.7)
    plt.legend(fontsize=10)
    
    out_dir = os.path.join(PROJECT_DIR, "plots")
    os.makedirs(out_dir, exist_ok=True)
    filename = os.path.join(out_dir, f'plot_{model_name.lower()}_{method}_{metric}.png')
    
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"[+] Generato: {os.path.relpath(filename, SCRIPT_DIR)}")

if __name__ == "__main__":
    # Cerca tutti i file di log per estrarre le diverse dimensioni (512, 1024, 2048)
    logs_dir = os.path.join(PROJECT_DIR, "logs")
    log_files = []
    
    if os.path.exists(logs_dir):
        for f in os.listdir(logs_dir):
            if f.startswith("strong_scaling_results_") and f.endswith("_finale.log"):
                log_files.append(os.path.join(logs_dir, f))
                
    if not log_files:
        print("[!] Errore: Nessun file 'strong_scaling_results_*_finale.log' trovato nella cartella logs.")
        sys.exit(1)
        
    print(f"[*] Trovati {len(log_files)} file di log. Parsing in corso...")
    parsed_data = parse_multiple_logs(log_files)
    
    models = ['OpenMP', 'MPI', 'Hybrid']
    methods = ['qr', 'potenze']
    
    for model in models:
        for method in methods:
            plot_metric(parsed_data, model, method, metric='time')
            plot_metric(parsed_data, model, method, metric='speedup')
            
    print("[*] Tutti i grafici sono stati aggiornati con successo.")
