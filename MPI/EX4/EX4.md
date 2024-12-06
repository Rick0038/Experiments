# EX4

## Task 1

### A) Writing the "first_mpi.c" program

We will write a simple MPI program where each process sends an integer to the next process in the chain. Each process increments the value by 1 before sending it to the next process. The first process initializes the integer value, and the message is passed from process 0 to the final process.

#### Code for "first_mpi.c"

```c
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>  // For sleep function

int main(int argc, char** argv) {
    int rank, size, tag = 0;
    int message = 0;
    MPI_Status status;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Each process sends a message to the next process in a chain
    if (rank == 0) {
        // Process 0 initializes the message
        message = 0;
        printf("I am Rank %d and I send Message %d\n", rank, message);
        
        // Increment the message and send it to the next process (rank 1)
        message++;
        MPI_Send(&message, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);
    } else {
        // All other processes receive a message first
        printf("I am Rank %d and I want to receive a message\n", rank);
        MPI_Recv(&message, 1, MPI_INT, rank - 1, tag, MPI_COMM_WORLD, &status);
        
        // Print the received message
        printf("I am Rank %d and I received Message %d\n", rank, message);
        
        // If not the last rank, send the message to the next process
        if (rank < size - 1) {
            message++;  // Increment the message
            printf("I am Rank %d and I send Message %d\n", rank, message);
            MPI_Send(&message, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);
        }
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
```

### B) Adding message announcements and simulation of sequential work with `sleep(1)`

We will modify the program slightly to include sleep statements to simulate sequential work. Every process will announce both when it starts receiving and when it sends a message. Additionally, we'll add a `sleep(1)` before each process sends a message to simulate sequential work.

```c
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>  // For sleep function

int main(int argc, char** argv) {
    int rank, size, tag = 0;
    int message = 0;
    MPI_Status status;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Each process sends a message to the next process in a chain
    if (rank == 0) {
        // Process 0 initializes the message
        message = 0;
        printf("I am Rank %d and I send Message %d\n", rank, message);
        
        sleep(1);  // Simulate work before sending
        // Increment the message and send it to the next process (rank 1)
        message++;
        MPI_Send(&message, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);
    } else {
        // All other processes receive a message first
        printf("I am Rank %d and I want to receive a message\n", rank);
        MPI_Recv(&message, 1, MPI_INT, rank - 1, tag, MPI_COMM_WORLD, &status);
        
        // Print the received message
        printf("I am Rank %d and I received Message %d\n", rank, message);
        
        // If not the last rank, send the message to the next process
        if (rank < size - 1) {
            sleep(1);  // Simulate work before sending
            message++;  // Increment the message
            printf("I am Rank %d and I send Message %d\n", rank, message);
            MPI_Send(&message, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);
        }
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
```

#### Behavior with `sleep(1)` before sending

- By adding a `sleep(1)` before sending, each process waits for a second before sending the message, simulating some work or processing delay. This will slow down the entire chain and make the process sequence visible when observed.

### C) Removing the `sleep(1)` before sending and adding it after every iteration

For this task, we will remove the `sleep(1)` before sending and add it after each iteration, i.e., after each process has sent or received a message.

```c
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>  // For sleep function

int main(int argc, char** argv) {
    int rank, size, tag = 0;
    int message = 0;
    MPI_Status status;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Each process sends a message to the next process in a chain
    if (rank == 0) {
        // Process 0 initializes the message
        message = 0;
        printf("I am Rank %d and I send Message %d\n", rank, message);
        
        // Increment the message and send it to the next process (rank 1)
        message++;
        MPI_Send(&message, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);
        
        sleep(1);  // Simulate work after sending
    } else {
        // All other processes receive a message first
        printf("I am Rank %d and I want to receive a message\n", rank);
        MPI_Recv(&message, 1, MPI_INT, rank - 1, tag, MPI_COMM_WORLD, &status);
        
        // Print the received message
        printf("I am Rank %d and I received Message %d\n", rank, message);
        
        // If not the last rank, send the message to the next process
        if (rank < size - 1) {
            message++;  // Increment the message
            printf("I am Rank %d and I send Message %d\n", rank, message);
            MPI_Send(&message, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);
        }
        sleep(1);  // Simulate work after sending/receiving
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
```

#### Behavior with `sleep(1)` after sending/receiving

- Now, after every iteration (either after sending or receiving), each process waits for a second. This will delay the progress of the next message being sent, and the chain will move slower overall but in a more synchronized manner.

### Conclusion

By adjusting where you place `sleep(1)`, you can simulate different types of delays and work in distributed systems, affecting how messages propagate through the ranks. By placing `sleep(1)` before sending, we introduce delays into the sending side, while placing it after sending/receiving synchronizes the overall pace but slows the chain down uniformly.
s