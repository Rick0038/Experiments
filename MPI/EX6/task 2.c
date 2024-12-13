#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int rank, size;
    int* send_array = NULL;
    int* recv_array = NULL;
    int* alltoall_array = NULL;
    int* result_array = NULL;
    int num_elements_per_proc = 4;  // Number of elements each process will receive
    
    MPI_Init(&argc, &argv);                // Initialize the MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Get the rank of the process
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Get the number of processes

    // Only process 0 initializes the full array
    if (rank == 0) {
        send_array = (int*)malloc(size * num_elements_per_proc * sizeof(int));
        for (int i = 0; i < size * num_elements_per_proc; i++) {
            send_array[i] = i + 1;  // Initialize array with 1, 2, 3, ..., size*num_elements_per_proc
        }
        printf("Process 0 initialized the array: ");
        for (int i = 0; i < size * num_elements_per_proc; i++) {
            printf("%d ", send_array[i]);
        }
        printf("\n");
    }

    // Each process allocates memory to receive its portion of the array
    recv_array = (int*)malloc(num_elements_per_proc * sizeof(int));
    
    // Scatter the array from process 0 to all processes
    MPI_Scatter(send_array, num_elements_per_proc, MPI_INT, recv_array, num_elements_per_proc, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Each process multiplies its received elements by 2
    result_array = (int*)malloc(num_elements_per_proc * sizeof(int));
    for (int i = 0; i < num_elements_per_proc; i++) {
        result_array[i] = recv_array[i] * 2;
    }
    
    // Allocate memory for Alltoall communication
    alltoall_array = (int*)malloc(size * num_elements_per_proc * sizeof(int));

    // Exchange arrays between all processes
    MPI_Alltoall(result_array, num_elements_per_proc, MPI_INT, alltoall_array, num_elements_per_proc, MPI_INT, MPI_COMM_WORLD);

    // Each process calculates the sum of all numbers received
    int sum = 0;
    for (int i = 0; i < size * num_elements_per_proc; i++) {
        sum += alltoall_array[i];
    }
    printf("Process %d: Sum of all elements received = %d\n", rank, sum);

    // Free dynamically allocated memory
    if (rank == 0) {
        free(send_array);
    }
    free(recv_array);
    free(result_array);
    free(alltoall_array);

    MPI_Finalize();  // Finalize the MPI environment
    return 0;
}
