# Variabili - Assicurati di usare il compilatore brew (es. gcc-15 se è quello che hai)
#CC = gcc-15		#versione per mac
CC = gcc			#versione per cluster

# Flag base (Ottimizzazioni, Vettorizzazione automatica, Fast Math)
CFLAGS_BASE = -Wall -O3 -g -march=native -ffast-math
LDFLAGS_BASE = -lm

# Flag specifici per attivare OpenMP
CFLAGS_OMP = $(CFLAGS_BASE) -fopenmp
LDFLAGS_OMP = $(LDFLAGS_BASE) -fopenmp

SRC_DIR = src
OBJ_DIR = obj

# Trova tutti i sorgenti
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Crea due liste di file oggetto separati per non sovrascriverli
OBJ_SERIAL = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%_serial.o, $(SOURCES))
OBJ_OMP = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%_omp.o, $(SOURCES))

EXEC_SERIAL = denoising_serial
EXEC_OMP = denoising_omp

# Regola principale: compila entrambi gli eseguibili
all: $(EXEC_SERIAL) $(EXEC_OMP)

# --- REGOLE PER LA VERSIONE SERIALE PURA ---
$(EXEC_SERIAL): $(OBJ_SERIAL)
	$(CC) $(CFLAGS_BASE) -o $@ $^ $(LDFLAGS_BASE)

$(OBJ_DIR)/%_serial.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS_BASE) -c $< -o $@

# --- REGOLE PER LA VERSIONE OPENMP ---
$(EXEC_OMP): $(OBJ_OMP)
	$(CC) $(CFLAGS_OMP) -o $@ $^ $(LDFLAGS_OMP)

$(OBJ_DIR)/%_omp.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS_OMP) -c $< -o $@

# Creazione cartella obj
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Pulizia
clean:
	rm -rf $(OBJ_DIR) $(EXEC_SERIAL) $(EXEC_OMP)

.PHONY: all clean