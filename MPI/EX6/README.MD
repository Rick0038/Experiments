### Task 1: MPI_Broadcast and MPI_Gather Example

This program receives an integer input from process 0, broadcasts it to all processes, each process increments the number by 5, and finally, the results are gathered by process 0 and printed.

```cpp
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
```

### Explanation of the Code:
1. **MPI_Bcast:** Process 0 broadcasts the input number to all other processes.
2. **MPI_Gather:** Each process sends its result back to process 0.
3. **Process 0** collects and prints the results.

---

### Task 2: MPI_Scatter and MPI_Alltoall Example

This program initializes an array on process 0, scatters it to all processes, each process multiplies the received elements by 2, and then uses `MPI_Alltoall` to exchange arrays between processes. Finally, each process calculates the sum of all arrays received.

```cpp
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
```

### Explanation of the Code:
1. **MPI_Scatter:** Process 0 scatters the initialized array to all processes.
2. Each process multiplies its portion of the array by 2.
3. **MPI_Alltoall:** Each process exchanges the modified array with all other processes.
4. Each process sums the elements of the received arrays and prints the result.

These two programs demonstrate how to use basic MPI functions like `MPI_Bcast`, `MPI_Gather`, `MPI_Scatter`, and `MPI_Alltoall` to distribute and gather data across multiple processes.