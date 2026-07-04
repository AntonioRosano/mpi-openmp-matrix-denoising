import re
import matplotlib.pyplot as plt
import os

log_file_path = '../logs/benchmark_mac_results_2048.log'
plot_output_dir = '../plots'

# Assicuriamoci che la cartella dei plot esista
os.makedirs(plot_output_dir, exist_ok=True)

# Dati da estrarre
sizes = []
naive_times = []
opt_times = []
blocked_times = []

# Variabili di stato per il parsing
current_size = None
current_version = None

# Espressioni regolari per il parsing
re_matrix_size = re.compile(r">>> TEST MATRICE: (\d+) x \d+ <<<")
re_version = re.compile(r"--- (\d)\. (.*?) ---")
re_time = re.compile(r"\[\*\] TEMPO TOTALE\s*: ([\d\.]+) secondi")

with open(log_file_path, 'r') as f:
    for line in f:
        # Cerca la dimensione della matrice
        match_size = re_matrix_size.search(line)
        if match_size:
            size = int(match_size.group(1))
            current_size = size
            continue
            
        # Salta la matrice 128 e tieni solo 512, 1024, 2048
        if current_size not in [512, 1024, 2048]:
            continue

        # Cerca il tipo di esecuzione (es. Seriale Naive, Opt, Opt Blocked)
        match_version = re_version.search(line)
        if match_version:
            version_num = int(match_version.group(1))
            version_name = match_version.group(2)
            if "Seriale Naive" in version_name:
                current_version = "naive"
            elif "Seriale Opt Blocked" in version_name:
                current_version = "blocked"
            elif "Seriale Opt" in version_name:
                current_version = "opt"
            else:
                current_version = None
            continue

        # Estrai il tempo totale
        match_time = re_time.search(line)
        if match_time and current_version:
            time_val = float(match_time.group(1))
            
            if current_size not in sizes:
                sizes.append(current_size)
                
            if current_version == "naive":
                naive_times.append(time_val)
            elif current_version == "opt":
                opt_times.append(time_val)
            elif current_version == "blocked":
                blocked_times.append(time_val)
                
            current_version = None # Reset dopo aver letto il tempo

# Preparazione del plot
x = range(len(sizes))
width = 0.25

fig, ax = plt.subplots(figsize=(10, 6))

# Plot delle barre
ax.bar([i - width for i in x], naive_times, width, label='Seriale Naive', color='#e74c3c', alpha=0.85)
ax.bar([i for i in x], opt_times, width, label='Seriale Opt', color='#3498db', alpha=0.85)
ax.bar([i + width for i in x], blocked_times, width, label='Seriale Opt Blocked', color='#2ecc71', alpha=0.85)

# Etichette, titolo e legenda
ax.set_xlabel('Dimensione Matrice (N)', fontsize=12)
ax.set_ylabel('Tempo Totale (secondi)', fontsize=12)
ax.set_title('Confronto Prestazioni: Naive vs Opt vs Opt Blocked', fontsize=14, pad=15)
ax.set_xticks(x)
ax.set_xticklabels([f"{s}x{s}" for s in sizes])
ax.legend()

# Scala logaritmica sull'asse Y per visualizzare meglio i gap prestazionali
ax.set_yscale('log')
# Aggiungiamo una griglia per migliore leggibilità
ax.grid(True, axis='y', linestyle='--', alpha=0.7)

# Etichette dei valori sopra le barre
def add_labels(rects):
    for rect in rects:
        height = rect.get_height()
        ax.annotate(f'{height:.2f}s',
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # offset verticale
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9, rotation=0)

# Chiamiamo add_labels solo se i valori non si sovrappongono troppo,
# ma con scala logaritmica è difficile, quindi usiamo un approccio basato sulle liste di patch:
for bar_container in ax.containers:
    add_labels(bar_container)

fig.tight_layout()

# Salvataggio del grafico
output_file = os.path.join(plot_output_dir, 'naive_vs_opt_vs_blocked.png')
plt.savefig(output_file, dpi=300)
print(f"[*] Grafico salvato con successo in {output_file}")
