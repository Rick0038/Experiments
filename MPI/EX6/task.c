#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MATRIX_SIZE 2

// Initializes a square matrix with random double values.
void initialize_matrix(double* matrix) {
    for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
        matrix[i] = (double)rand() / RAND_MAX;
    }
}

// Displays a square matrix to the console.
void display_matrix(double* matrix) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            printf("%.2f ", matrix[i * MATRIX_SIZE + j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Computes the source rank for matrix A's shift.
void compute_source_rank_a(int my_rank, int grid_dimension, int shift, int* source_rank) {
    *source_rank = my_rank + shift;
    if ((my_rank % grid_dimension) + shift >= grid_dimension) {
        *source_rank -= grid_dimension;
    }
}

// Computes the destination rank for matrix A's shift.
void compute_destination_rank_a(int my_rank, int grid_dimension, int shift, int* destination_rank) {
    *destination_rank = my_rank - shift;
    if ((my_rank % grid_dimension) - shift < 0) {
        *destination_rank += grid_dimension;
    }
}

// Computes the source rank for matrix B's shift.
void compute_source_rank_b(int my_rank, int grid_dimension, int total_size, int shift, int* source_rank) {
    *source_rank = my_rank + (shift * grid_dimension);
    if ((my_rank / grid_dimension) + shift >= grid_dimension) {
        *source_rank -= total_size;
    }
}

// Computes the destination rank for matrix B's shift.
void compute_destination_rank_b(int my_rank, int grid_dimension, int total_size, int shift, int* destination_rank) {
    *destination_rank = my_rank - (shift * grid_dimension);
    if ((my_rank / grid_dimension) - shift < 0) {
        *destination_rank += total_size;
    }
}

int main(int argc, char* argv[]) {
    int my_rank, total_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &total_size);

    double* matrix_a = NULL;
    double* matrix_b = NULL;
    double local_value_a, local_value_b, local_value_c = 0.0;

    if (my_rank == 0) {
        matrix_a = (double*)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        matrix_b = (double*)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));

        initialize_matrix(matrix_a);
        initialize_matrix(matrix_b);

        printf("Matrix A:\n");
        display_matrix(matrix_a);

        printf("Matrix B:\n");
        display_matrix(matrix_b);
    }

    MPI_Scatter(matrix_a, 1, MPI_DOUBLE, &local_value_a, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatter(matrix_b, 1, MPI_DOUBLE, &local_value_b, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Status status;
    int grid_dimension = sqrt(total_size);
    int row = my_rank / grid_dimension;
    int col = my_rank % grid_dimension;

    for (int step = 0; step < grid_dimension; step++) {
        int destination_rank_a, source_rank_a, shift_a;
        shift_a = (step == 0) ? row : 1;

        compute_destination_rank_a(my_rank, grid_dimension, shift_a, &destination_rank_a);
        compute_source_rank_a(my_rank, grid_dimension, shift_a, &source_rank_a);

        MPI_Sendrecv_replace(&local_value_a, 1, MPI_DOUBLE, destination_rank_a, 0, source_rank_a, 0, MPI_COMM_WORLD, &status);

        int destination_rank_b, source_rank_b, shift_b;
        shift_b = (step == 0) ? col : 1;

        compute_destination_rank_b(my_rank, grid_dimension, total_size, shift_b, &destination_rank_b);
        compute_source_rank_b(my_rank, grid_dimension, total_size, shift_b, &source_rank_b);

        MPI_Sendrecv_replace(&local_value_b, 1, MPI_DOUBLE, destination_rank_b, 0, source_rank_b, 0, MPI_COMM_WORLD, &status);

        local_value_c += local_value_a * local_value_b;
    }

    double* final_matrix_c = NULL;
    if (my_rank == 0) {
        final_matrix_c = (double*)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
    }

    MPI_Gather(&local_value_c, 1, MPI_DOUBLE, final_matrix_c, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        printf("Matrix C (Result of A x B):\n");
        display_matrix(final_matrix_c);
        free(final_matrix_c);
    }

    if (my_rank == 0) {
        free(matrix_a);
        free(matrix_b);
    }

    MPI_Finalize();
    return 0;
}