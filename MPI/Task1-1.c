#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 8  // Total size of the 1D array

int main(int argc, char* argv[]) {
    int rank, size;

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (N % size != 0) {
        if (rank == 0) {
            printf("Error: Array size N must be divisible by the number of processes.\n");
        }
        MPI_Finalize();
        exit(1);
    }

    int elements_per_proc = N / size;  // Number of elements per process
    int *global_array = NULL;
    int *local_array = (int*)malloc(elements_per_proc * sizeof(int));  // Local array for each process

    if (rank == 0) {
        // Allocate and initialize the global array on rank 0
        global_array = (int*)malloc(N * sizeof(int));
        for (int i = 0; i < N; i++) {
            global_array[i] = i + 1;  // Example: global_array = {1, 2, 3, ..., N}
        }

        printf("Global array before scatter:\n");
        for (int i = 0; i < N; i++) {
            printf("%d ", global_array[i]);
        }
        printf("\n\n");
    }

    // Scatter the global array to all processes
    MPI_Scatter(global_array, elements_per_proc, MPI_INT, local_array, elements_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

    // Print local array after scatter
    printf("Process %d received: ", rank);
    for (int i = 0; i < elements_per_proc; i++) {
        printf("%d ", local_array[i]);
    }
    printf("\n");

    // Shift local array (shift elements to the left within each process, for demonstration)
    int tmp = local_array[0];
    for (int i = 0; i < elements_per_proc - 1; i++) {
        local_array[i] = local_array[i + 1];
    }
    local_array[elements_per_proc - 1] = tmp;

    // Perform task: element-wise multiplication by 2
    for (int i = 0; i < elements_per_proc; i++) {
        local_array[i] *= 2;
    }

    // Gather the modified local arrays back into the global array on rank 0
    MPI_Gather(local_array, elements_per_proc, MPI_INT, global_array, elements_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

    // Print the result on rank 0
    if (rank == 0) {
        printf("\nGlobal array after gather (shifted and multiplied by 2):\n");
        for (int i = 0; i < N; i++) {
            printf("%d ", global_array[i]);
        }
        printf("\n");
        free(global_array);
    }

    // Clean up
    free(local_array);

    // Finalize MPI
    MPI_Finalize();
    return 0;
}
