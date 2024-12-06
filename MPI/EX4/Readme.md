# EX4

## Task 1

### Steps to Compile and Run the MPI Program

#### Step 1: Install MPI

Before compiling and running an MPI program, you need to install the MPI library. We will use **OpenMPI**, which is commonly used.

For **Ubuntu/Debian**:

```bash
sudo apt update
sudo apt install mpich
```

For **CentOS/RHEL**:

```bash
sudo yum install openmpi openmpi-devel
```

For **MacOS**:
You can install MPI using Homebrew:

```bash
brew install open-mpi
```

After installing MPI, you need to load the MPI environment in your shell. On most systems, you can do this by adding the following to your `.bashrc` or `.zshrc`:

```bash
export PATH=$PATH:/usr/lib64/openmpi/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64/openmpi/lib
```

Then, reload the shell:

```bash
source ~/.bashrc  # Or ~/.zshrc if you're using Zsh
```

#### Step 2: Create the MPI Program

1. Open a terminal and create a new directory for your MPI project:

   ```bash
   mkdir mpi_project
   cd mpi_project
   ```

2. Create a file called `first_mpi.c` using your favorite text editor (e.g., `nano`, `vim`):

   ```bash
   nano first_mpi.c
   ```

3. Paste the code you want to compile (choose A, B, or C code provided earlier in our conversation).

4. Save the file and exit the editor.

#### Step 3: Compile the Program

Use the MPI compiler `mpicc` to compile your program. Here's the command to compile `first_mpi.c`:

```bash
mpicc -o first_mpi first_mpi.c
```

- This command will generate an executable called `first_mpi` in the current directory.
- If there are any errors, ensure you have correctly pasted the code and installed MPI.

#### Step 4: Run the MPI Program

To run the program with multiple processes, use `mpirun` or `mpiexec`. For example, if you want to run the program with 4 processes:

```bash
mpirun -np 4 ./first_mpi
```

- `-np 4` specifies that 4 processes will be used.
- `./first_mpi` is the compiled executable.

#### Expected Output

#### A) Basic Chain of Incremented Message Passing (Without Sleep)

If you use the first version of the code, where each process increments the message and passes it to the next, you will see the following output (assuming 4 processes):

```sh
I am Rank 0 and I send Message 0
I am Rank 1 and I want to receive a message
I am Rank 1 and I received Message 1
I am Rank 1 and I send Message 2
I am Rank 2 and I want to receive a message
I am Rank 2 and I received Message 2
I am Rank 2 and I send Message 3
I am Rank 3 and I want to receive a message
I am Rank 3 and I received Message 3
```

- Process 0 starts the chain by sending the initial message `0`.
- Process 1 receives the message, increments it, and sends `2` to process 2.
- Process 2 receives the message `2`, increments it, and sends `3` to process 3.
- Process 3 receives the final message `3`.

#### B) Adding `sleep(1)` Before Sending

If you use the modified version of the code (B), where `sleep(1)` is added **before sending** the message, the output will be similar but slower. The program will pause for 1 second before each process sends its message, which gives the effect of sequential work:

```sh
I am Rank 0 and I send Message 0
(I wait for 1 second...)
I am Rank 1 and I want to receive a message
I am Rank 1 and I received Message 1
I am Rank 1 and I send Message 2
(I wait for 1 second...)
I am Rank 2 and I want to receive a message
I am Rank 2 and I received Message 2
I am Rank 2 and I send Message 3
(I wait for 1 second...)
I am Rank 3 and I want to receive a message
I am Rank 3 and I received Message 3
```

- The output will appear more slowly because there is a 1-second delay after each process announces it is sending the message.
- You'll notice the overall process takes longer.

#### C) Adding `sleep(1)` After Every Iteration

If you use the final version (C) where `sleep(1)` is added **after sending/receiving**, the output will be slower, but the delay happens uniformly after each process has completed its task:

```sh
I am Rank 0 and I send Message 0
I am Rank 1 and I want to receive a message
I am Rank 1 and I received Message 1
I am Rank 1 and I send Message 2
(I wait for 1 second...)
I am Rank 2 and I want to receive a message
I am Rank 2 and I received Message 2
I am Rank 2 and I send Message 3
(I wait for 1 second...)
I am Rank 3 and I want to receive a message
I am Rank 3 and I received Message 3
```

- The program still delays after each message passing, but the delay happens after the message is sent/received rather than before.
- This method creates a more synchronized behavior in the chain because each rank processes its task and then waits.

### Step 5: Analyzing the Behavior

1. **Without `sleep(1)`**: The message-passing happens as quickly as possible, and the program finishes fast. Each rank receives and sends a message almost immediately after the previous rank completes.

2. **With `sleep(1)` before sending**: Adding the sleep before sending creates a visible delay before each process sends its message. This simulates "work" happening in between and gives a staggered execution, where you can clearly see the progression of the message through each process.

3. **With `sleep(1)` after sending/receiving**: This adds a pause after each rank has finished its task, synchronizing the process delays. The message chain will proceed more slowly because each rank waits after completing its task, creating uniform pacing.

### Troubleshooting Tips

- If the program doesn't run, make sure MPI is correctly installed by running `mpicc --version` and `mpirun --version`.
- Ensure the file `first_mpi.c` was compiled without errors.
- If you see a segmentation fault or other MPI-related errors, check that you are running the correct number of processes and that the MPI environment is correctly initialized.

That's it! Following these steps will allow you to compile and run the MPI program on your local machine and observe the different behaviors.
