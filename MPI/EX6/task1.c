#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int number, result;
    
    MPI_Init(&argc, &argv);             // Initialize the MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank of the process
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get the number of processes

    if (rank == 0) {
        // Process 0 inputs the number
        printf("Enter a number: ");
        scanf("%d", &number);
    }

    // Broadcast the number from process 0 to all other processes
    MPI_Bcast(&number, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Each process increments the number by 5
    result = number + 5;
    printf("Process %d: Number after incrementing by 5 = %d\n", rank, result);
    
    // Gather all results from all processes to process 0
    int* results = NULL;
    if (rank == 0) {
        results = (int*)malloc(size * sizeof(int)); // Allocate memory for receiving gathered results
    }

    MPI_Gather(&result, 1, MPI_INT, results, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Process 0 prints all gathered results
    if (rank == 0) {
        printf("Results gathered at process 0:\n");
        for (int i = 0; i < size; i++) {
            printf("Result from process %d: %d\n", i, results[i]);
        }
        free(results); // Free allocated memory
    }

    MPI_Finalize();  // Finalize the MPI environment
    return 0;
}
