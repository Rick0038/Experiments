#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
Reference 
https://mpitutorial.com/tutorials/introduction-to-groups-and-communicators/
https://wgropp.cs.illinois.edu/courses/cs598-s15/lectures/lecture28.pdf
https://www3.nd.edu/~zxu2/acms60212-40212-S12/Lec-08-1.pdf
http://www.rc.usf.edu/tutorials/classes/tutorial/mpi/chapter10.html
*/

#define N 2 // Matrix dimension (N x N)

MPI_Comm cart_comm; // Global Cartesian communicator

// Utility function to print a matrix
void printMatrix(double* mat, int n, const char* name) {
    printf("Matrix %s:\n", name);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%f ", mat[i*n + j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Function to initialize MPI and return rank and size
void initialize_MPI(int *rank, int *size, int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, rank);
    MPI_Comm_size(MPI_COMM_WORLD, size);
}

// Function to set up the Cartesian grid, return coordinates, neighbors (left, right, up, down)
void setup_cartesian_grid(int rank, int size, int coords[2], int *left, int *right, int *up, int *down) {
    int dims[2], periods[2];
    
    // Ensure the number of processes matches N^2
    if (size != N * N) {
        if (rank == 0) {
            printf("Error: The number of processes must be %d for a %dx%d matrix\n", N * N, N, N);
        }
        MPI_Finalize();
        exit(-1);
    }

    // Create a 2D Cartesian grid
    dims[0] = dims[1] = N;
    periods[0] = periods[1] = 1; // Enable wrap-around for Cannon's algorithm
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 2, coords); // Get the coordinates of this rank in the grid

    // Find neighboring ranks in the Cartesian grid
    MPI_Cart_shift(cart_comm, 1, -1, right, left); // Shift left/right
    MPI_Cart_shift(cart_comm, 0, -1, down, up);    // Shift up/down
}

// Function to perform matrix multiplication using Cannon's algorithm
void matrix_multiplication_cannon(int coords[2], int left, int right, int up, int down, double A_elem, double B_elem, double *C_elem) {
    // Initial alignment of A and B (Cannon's step 1)
    for (int i = 0; i < coords[0]; i++) {
        MPI_Sendrecv_replace(&A_elem, 1, MPI_DOUBLE, left, 0, right, 0, cart_comm, MPI_STATUS_IGNORE);
    }
    for (int i = 0; i < coords[1]; i++) {
        MPI_Sendrecv_replace(&B_elem, 1, MPI_DOUBLE, up, 0, down, 0, cart_comm, MPI_STATUS_IGNORE);
    }

    // Perform the multiplication and shifting (Cannon's steps 2 and 3)
    for (int step = 0; step < N; step++) {
        *C_elem += A_elem * B_elem; // Compute the product and accumulate
        
        // Shift A left and B up
        MPI_Sendrecv_replace(&A_elem, 1, MPI_DOUBLE, left, 0, right, 0, cart_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv_replace(&B_elem, 1, MPI_DOUBLE, up, 0, down, 0, cart_comm, MPI_STATUS_IGNORE);
    }
}

// Function to finalize MPI
void finalize_MPI() {
    MPI_Finalize();
}

int main(int argc, char* argv[]) {
    int rank, size, coords[2], left, right, up, down;
    double A_elem, B_elem, C_elem = 0.0;
    
    // Initialize MPI
    initialize_MPI(&rank, &size, argc, argv);

    // Setup Cartesian grid
    setup_cartesian_grid(rank, size, coords, &left, &right, &up, &down);
    
    // Initialize matrix elements (for simplicity, based on coordinates)
    A_elem = (double)(coords[0] * N + coords[1] + 1); // value for A
    B_elem = (double)(coords[0] * N + coords[1] + 1); // value for B

    // Buffers to gather all matrix elements on rank 0
    double* A = NULL;
    double* B = NULL;
    if (rank == 0) {
        A = (double*)malloc(N * N * sizeof(double));
        B = (double*)malloc(N * N * sizeof(double));
    }

    // Gather matrix A and B elements on rank 0 for printing
    MPI_Gather(&A_elem, 1, MPI_DOUBLE, A, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&B_elem, 1, MPI_DOUBLE, B, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Print matrices A and B on rank 0 after Cartesian grid setup
    if (rank == 0) {
        printMatrix(A, N, "A");
        printMatrix(B, N, "B");
    }

    // Perform matrix multiplication using Cannon's algorithm
    matrix_multiplication_cannon(coords, left, right, up, down, A_elem, B_elem, &C_elem);

    // Buffer to gather result matrix C
    double* C = NULL;
    if (rank == 0) {
        C = (double*)malloc(N * N * sizeof(double));
    }
    MPI_Gather(&C_elem, 1, MPI_DOUBLE, C, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Print the result matrix C on rank 0
    if (rank == 0) {
        printMatrix(C, N, "C");
        free(A);
        free(B);
        free(C);
    }

    // Finalize MPI
    finalize_MPI();
    return 0;
}

