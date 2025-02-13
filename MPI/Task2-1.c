#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Define the global matrix size 
#define MATRIX_SIZE 800

// Fill array
void fillArray(double* arr, int n) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int random = rand();
            arr[i * n + j] = (double)random / RAND_MAX;
        }
    }
}

// multiply local block 
void localMultiply(double *A, double *B, double *C, int blockSize) {
    for (int i = 0; i < blockSize; i++) {
        for (int j = 0; j < blockSize; j++) {
            for (int k = 0; k < blockSize; k++) {
                C[i * blockSize + j] += A[i * blockSize + k] * B[k * blockSize + j];
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // square grid check
    int gridDim = (int)sqrt(size);
    if (gridDim * gridDim != size) {
        if (rank == 0)
            fprintf(stderr, "Error: Number of processes (%d) is not a square.\n", size);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    
    // set the block size
    int blockSize = MATRIX_SIZE / gridDim;
    
    // create cartesian grid
    int dims[2] = {gridDim, gridDim};
    int periods[2] = {1, 1}; 
    MPI_Comm cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart);
    
    // coordinates of this process in the grid.
    int coords[2];
    MPI_Cart_coords(cart, rank, 2, coords);
    int myRow = coords[0];
    int myCol = coords[1];
    
    // allocate memory 
    double *A_local = (double*)malloc(blockSize * blockSize * sizeof(double));
    double *B_local = (double*)malloc(blockSize * blockSize * sizeof(double));
    double *C_local = (double*)malloc(blockSize * blockSize * sizeof(double));

    for (int i = 0; i < blockSize * blockSize; i++) {
        C_local[i] = 0.0;
    }
    
    double *A = NULL, *B = NULL, *C = NULL;
    if (rank == 0) {
        A = (double*)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        B = (double*)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        C = (double*)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        fillArray(A, MATRIX_SIZE);
        fillArray(B, MATRIX_SIZE);
        
        printf("Matrix A:\n");
        for (int i = 0; i < MATRIX_SIZE; i++) {
            for (int j = 0; j < MATRIX_SIZE; j++)
                printf("%f ", A[i * MATRIX_SIZE + j]);
            printf("\n");
        }
        printf("Matrix B:\n");
        for (int i = 0; i < MATRIX_SIZE; i++) {
            for (int j = 0; j < MATRIX_SIZE; j++)
                printf("%f ", B[i * MATRIX_SIZE + j]);
            printf("\n");
        }
    }
    
    /* 
       === Distribution of Blocks (Checkerboard) ===
       Instead of using MPI derived datatypes, we manually extract each block.
       The root process loops over all grid positions, extracts the corresponding
       submatrix (of size blockSize x blockSize) from the global matrix,
       and sends it to the correct process.
    */
    if (rank == 0) {
        // Distribute matrix A block by block.
        for (int i = 0; i < gridDim; i++) {
            for (int j = 0; j < gridDim; j++) {
                int destRank;
                int destCoords[2] = {i, j};
                MPI_Cart_rank(cart, destCoords, &destRank);
                
                // Allocate a temporary buffer for the block.
                double *tempBlock = (double*)malloc(blockSize * blockSize * sizeof(double));
                for (int bi = 0; bi < blockSize; bi++) {
                    for (int bj = 0; bj < blockSize; bj++) {
                        int globalRow = i * blockSize + bi;
                        int globalCol = j * blockSize + bj;
                        tempBlock[bi * blockSize + bj] = A[globalRow * MATRIX_SIZE + globalCol];
                    }
                }
                
                if (destRank == 0) {
                    // For the root process, copy directly into A_local.
                    for (int k = 0; k < blockSize * blockSize; k++)
                        A_local[k] = tempBlock[k];
                } else {
                    MPI_Send(tempBlock, blockSize * blockSize, MPI_DOUBLE, destRank, 0, MPI_COMM_WORLD);
                }
                free(tempBlock);
            }
        }
        // Distribute matrix B in the same way.
        for (int i = 0; i < gridDim; i++) {
            for (int j = 0; j < gridDim; j++) {
                int destRank;
                int destCoords[2] = {i, j};
                MPI_Cart_rank(cart, destCoords, &destRank);
                
                double *tempBlock = (double*)malloc(blockSize * blockSize * sizeof(double));
                for (int bi = 0; bi < blockSize; bi++) {
                    for (int bj = 0; bj < blockSize; bj++) {
                        int globalRow = i * blockSize + bi;
                        int globalCol = j * blockSize + bj;
                        tempBlock[bi * blockSize + bj] = B[globalRow * MATRIX_SIZE + globalCol];
                    }
                }
                
                if (destRank == 0) {
                    for (int k = 0; k < blockSize * blockSize; k++)
                        B_local[k] = tempBlock[k];
                } else {
                    MPI_Send(tempBlock, blockSize * blockSize, MPI_DOUBLE, destRank, 0, MPI_COMM_WORLD);
                }
                free(tempBlock);
            }
        }
    } else {
        // All non-root processes receive their blocks for A and B.
        MPI_Recv(A_local, blockSize * blockSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(B_local, blockSize * blockSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    /*
       === Cannon’s Algorithm: Initial Alignment ===
       For matrix A, each process shifts its block left by its row coordinate.
       For matrix B, each process shifts its block upward by its column coordinate.
       These shifts are performed with wrap-around (cyclic) behavior.
    */
    MPI_Status status;
    int src, dest;
    
    // Shift A_local left by myRow positions.
    for (int i = 0; i < myRow; i++) {
        MPI_Cart_shift(cart, 1, -1, &src, &dest);
        MPI_Sendrecv_replace(A_local, blockSize * blockSize, MPI_DOUBLE,
                             dest, 0, src, 0, cart, &status);
    }
    // Shift B_local upward by myCol positions.
    for (int i = 0; i < myCol; i++) {
        MPI_Cart_shift(cart, 0, -1, &src, &dest);
        MPI_Sendrecv_replace(B_local, blockSize * blockSize, MPI_DOUBLE,
                             dest, 0, src, 0, cart, &status);
    }
    
    /*
       === Main Loop of Cannon's Algorithm ===
       In each of gridDim steps, each process:
         1. Multiplies its local blocks A_local and B_local and accumulates the result in C_local.
         2. Shifts A_local one step to the left.
         3. Shifts B_local one step upward.
    */
    for (int step = 0; step < gridDim; step++) {
        // Multiply the current local blocks and add to C_local.
        localMultiply(A_local, B_local, C_local, blockSize);
        
        // Shift A_local left by one position.
        MPI_Cart_shift(cart, 1, -1, &src, &dest);
        MPI_Sendrecv_replace(A_local, blockSize * blockSize, MPI_DOUBLE,
                             dest, 0, src, 0, cart, &status);
        // Shift B_local upward by one position.
        MPI_Cart_shift(cart, 0, -1, &src, &dest);
        MPI_Sendrecv_replace(B_local, blockSize * blockSize, MPI_DOUBLE,
                             dest, 0, src, 0, cart, &status);
    }
    
    /*
       === Gathering the Result ===
       Each process’s computed block (C_local) is sent back to the root process,
       which reassembles the global result matrix C.
    */
    if (rank == 0) {
        // The root process receives each block and places it in the correct position.
        for (int i = 0; i < gridDim; i++) {
            for (int j = 0; j < gridDim; j++) {
                int srcRank;
                int srcCoords[2] = {i, j};
                MPI_Cart_rank(cart, srcCoords, &srcRank);
                
                // Temporary buffer to hold the received block.
                double *tempBlock = (double*)malloc(blockSize * blockSize * sizeof(double));
                if (srcRank == 0) {
                    // For rank 0, simply copy from C_local.
                    for (int k = 0; k < blockSize * blockSize; k++)
                        tempBlock[k] = C_local[k];
                } else {
                    MPI_Recv(tempBlock, blockSize * blockSize, MPI_DOUBLE, srcRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                
                // Copy the block into the global matrix C.
                for (int bi = 0; bi < blockSize; bi++) {
                    for (int bj = 0; bj < blockSize; bj++) {
                        int globalRow = i * blockSize + bi;
                        int globalCol = j * blockSize + bj;
                        C[globalRow * MATRIX_SIZE + globalCol] = tempBlock[bi * blockSize + bj];
                    }
                }
                free(tempBlock);
            }
        }
    } else {
        // Non-root processes send their computed block C_local to the root.
        MPI_Send(C_local, blockSize * blockSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
    
    // Root process prints the final result matrix.
    if (rank == 0) {
        printf("Result Matrix C (A x B):\n");
        for (int i = 0; i < MATRIX_SIZE; i++) {
            for (int j = 0; j < MATRIX_SIZE; j++)
                printf("%f ", C[i * MATRIX_SIZE + j]);
            printf("\n");
        }
    }
    
    // Clean up.
    free(A_local);
    free(B_local);
    free(C_local);
    if (rank == 0) {
        free(A);
        free(B);
        free(C);
    }
    MPI_Comm_free(&cart);
    MPI_Finalize();
    return 0;
}
