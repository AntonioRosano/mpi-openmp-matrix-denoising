CC = gcc-15
MPICC = mpicc
CFLAGS = -Wall -O3 -march=native -ffast-math
LDFLAGS = -lm

OMP_FLAGS = -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
OMP_LIBS = -L/opt/homebrew/opt/libomp/lib -lomp

ALL_SRC = src/main.c src/io_utils.c src/verifier.c src/methods_naive.c src/methods_opt.c src/methods_blocked.c

all: denoising_serial denoising_omp denoising_mpi

denoising_serial:
	$(CC) $(CFLAGS) -o denoising_serial $(ALL_SRC) $(LDFLAGS)

denoising_omp:
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o denoising_omp $(ALL_SRC) $(LDFLAGS) $(OMP_LIBS)

denoising_mpi:
	$(MPICC) $(CFLAGS) $(OMP_FLAGS) -o denoising_mpi src/main_mpi.c src/methods_mpi.c src/methods_opt.c src/io_utils.c src/verifier.c $(LDFLAGS) $(OMP_LIBS)

clean:
	rm -f denoising_serial denoising_omp denoising_mpi

.PHONY: all clean