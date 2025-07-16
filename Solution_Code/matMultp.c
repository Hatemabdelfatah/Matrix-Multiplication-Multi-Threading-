#include <stdio.h>      // Standard I/O functions (printf, fscanf, etc.)
#include <stdlib.h>     // Memory allocation and general utilities (malloc, free, exit)
#include <string.h>     // String handling functions (strcpy, strcat)
#include <pthread.h>    // POSIX threads for multi-threading
#include <sys/time.h>   // gettimeofday() for measuring execution time

// Global variables for matrix dimensions and pointers to matrices
int A_rows, A_cols, B_rows, B_cols;
int **A, **B;
int **C1, **C2, **C3; // C1: result for method 1, C2: for method 2, C3: for method 3

// Function to allocate memory for a 2D matrix of given rows and columns
int **allocateMatrix(int rows, int cols) {
    int **mat = (int **) malloc(rows * sizeof(int *));
    if (mat == NULL) {
        printf("Memory allocation error\n");
        exit(1);
    }
    // Allocate memory for each row
    for (int i = 0; i < rows; i++) {
        mat[i] = (int *) malloc(cols * sizeof(int));
        if (mat[i] == NULL) {
            printf("Memory allocation error\n");
            exit(1);
        }
    }
    return mat;
}

// Function to free memory allocated for a 2D matrix
void freeMatrix(int **mat, int rows) {
    for (int i = 0; i < rows; i++) {
        free(mat[i]);  // Free each row
    }
    free(mat);         // Free the array of pointers
}

// Function to read a matrix from a file.
// Expects first line of the file to have "row=x col=y" format.
int **readMatrix(const char *filename, int *rows, int *cols) {
    FILE *fp = fopen(filename, "r");  // Open file for reading
    if (fp == NULL) {
        printf("Error opening file %s\n", filename);
        exit(1);
    }
    char line[256];
    fgets(line, sizeof(line), fp);    // Read the first line containing dimensions
    sscanf(line, "row=%d col=%d", rows, cols);  // Parse rows and columns
    int **mat = allocateMatrix(*rows, *cols);   // Allocate memory for the matrix
    // Read the matrix values from the file
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            fscanf(fp, "%d", &mat[i][j]);
        }
    }
    fclose(fp);  // Close the file after reading
    return mat;
}

// Function to write a matrix to a file in the required format
void writeMatrix(const char *filename, int **mat, int rows, int cols) {
    FILE *fp = fopen(filename, "w");  // Open file for writing
    if (fp == NULL) {
        printf("Error opening file %s for writing\n", filename);
        exit(1);
    }
    fprintf(fp, "row=%d col=%d\n", rows, cols);  // Write dimensions
    // Write the matrix values, row by row
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(fp, "%d ", mat[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);  // Close the file after writing
}

//-------------------------------------------------------------------//
// Method 1: Single thread computes the entire output matrix.
//-------------------------------------------------------------------//
void *multiplyMatrix(void *arg) {
    int i, j, k;
    // Loop over each row of matrix A and column of matrix B
    for (i = 0; i < A_rows; i++) {
        for (j = 0; j < B_cols; j++) {
            int sum = 0;
            // Compute the dot product for element (i, j)
            for (k = 0; k < A_cols; k++) {
                sum += A[i][k] * B[k][j];
            }
            C1[i][j] = sum;  // Store result in C1
        }
    }
    pthread_exit(NULL);  // Terminate the thread
}

//-------------------------------------------------------------------//
// Method 2: One thread per row of the output matrix.
//-------------------------------------------------------------------//

// Structure to pass row number to each thread
typedef struct {
    int row;
} threadRowArg;

void *multiplyRow(void *arg) {
    threadRowArg *data = (threadRowArg *) arg;
    int i = data->row;
    int j, k;
    // Compute one entire row of the output matrix
    for (j = 0; j < B_cols; j++) {
        int sum = 0;
        for (k = 0; k < A_cols; k++) {
            sum += A[i][k] * B[k][j];
        }
        C2[i][j] = sum;  // Store result in C2
    }
    free(data);          // Free dynamically allocated argument
    pthread_exit(NULL);  // Terminate the thread
}

//-------------------------------------------------------------------//
// Method 3: One thread per element of the output matrix.
//-------------------------------------------------------------------//

// Structure to pass both row and column numbers to each thread
typedef struct {
    int row;
    int col;
} threadElemArg;

void *multiplyElement(void *arg) {
    threadElemArg *data = (threadElemArg *) arg;
    int i = data->row;
    int j = data->col;
    int k, sum = 0;
    // Compute the dot product for a single element
    for (k = 0; k < A_cols; k++) {
        sum += A[i][k] * B[k][j];
    }
    C3[i][j] = sum;  // Store result in C3
    free(data);      // Free dynamically allocated argument
    pthread_exit(NULL);  // Terminate the thread
}

//-------------------------------------------------------------------//
// Main function: Handles input, thread creation, timing, and output.
//-------------------------------------------------------------------//
int main(int argc, char *argv[]) {
    // Variables to store file names and output prefix
    char inputFile1[100], inputFile2[100], outputPrefix[100];
    
    // Check command-line arguments: if none, use default file names.
    if (argc == 1) {
        strcpy(inputFile1, "a.txt");
        strcpy(inputFile2, "b.txt");
        strcpy(outputPrefix, "c");
    } else if (argc == 4) {
        // Append ".txt" to input filenames provided as arguments
        strcpy(inputFile1, argv[1]);
        strcat(inputFile1, ".txt");
        strcpy(inputFile2, argv[2]);
        strcat(inputFile2, ".txt");
        strcpy(outputPrefix, argv[3]);
    } else {
        // Incorrect usage, print message and exit
        printf("Usage: %s [Mat1 Mat2 MatOut]\n", argv[0]);
        exit(1);
    }
    
    // Read matrices A and B from the input files
    A = readMatrix(inputFile1, &A_rows, &A_cols);
    B = readMatrix(inputFile2, &B_rows, &B_cols);
    
    // Check if matrix multiplication is possible (A_cols must equal B_rows)
    if (A_cols != B_rows) {
        printf("Matrix multiplication not possible. A_cols != B_rows\n");
        exit(1);
    }
    
    // Define the dimensions of the result matrix
    int resultRows = A_rows;
    int resultCols = B_cols;
    
    // Allocate memory for each of the three result matrices
    C1 = allocateMatrix(resultRows, resultCols);
    C2 = allocateMatrix(resultRows, resultCols);
    C3 = allocateMatrix(resultRows, resultCols);
    
    // Variables to measure execution time
    struct timeval start, stop;
    long seconds, microseconds;
    
    //-------------------------------------------------------------------//
    // Method 1: One thread computes the entire matrix.
    //-------------------------------------------------------------------//
    pthread_t thread1;
    gettimeofday(&start, NULL);  // Start timing for Method 1
    if (pthread_create(&thread1, NULL, multiplyMatrix, NULL)) {
        printf("Error creating thread for method 1\n");
        exit(1);
    }
    if (pthread_join(thread1, NULL)) {  // Wait for thread to finish
        printf("Error joining thread for method 1\n");
        exit(1);
    }
    gettimeofday(&stop, NULL);  // End timing for Method 1
    seconds = stop.tv_sec - start.tv_sec;
    microseconds = stop.tv_usec - start.tv_usec;
    // Print the thread count and execution time for Method 1
    printf("Method 1 (per matrix): Threads = 1, Time taken: %ld seconds and %ld microseconds\n", seconds, microseconds);
    
    // Write result matrix C1 to file
    char filename1[150];
    sprintf(filename1, "%s_per_matrix.txt", outputPrefix);
    writeMatrix(filename1, C1, resultRows, resultCols);
    
    //-------------------------------------------------------------------//
    // Method 2: One thread per row.
    //-------------------------------------------------------------------//
    pthread_t threads2[resultRows];
    gettimeofday(&start, NULL);  // Start timing for Method 2
    for (int i = 0; i < resultRows; i++) {
        // Allocate and set the thread argument for this row
        threadRowArg *arg = (threadRowArg *) malloc(sizeof(threadRowArg));
        arg->row = i;
        if (pthread_create(&threads2[i], NULL, multiplyRow, arg)) {
            printf("Error creating thread for row %d in method 2\n", i);
            exit(1);
        }
    }
    // Wait for all row threads to complete
    for (int i = 0; i < resultRows; i++) {
        if (pthread_join(threads2[i], NULL)) {
            printf("Error joining thread for row %d in method 2\n", i);
            exit(1);
        }
    }
    gettimeofday(&stop, NULL);  // End timing for Method 2
    seconds = stop.tv_sec - start.tv_sec;
    microseconds = stop.tv_usec - start.tv_usec;
    // Print the thread count (number of rows) and execution time for Method 2
    printf("Method 2 (per row): Threads = %d, Time taken: %ld seconds and %ld microseconds\n", resultRows, seconds, microseconds);
    
    // Write result matrix C2 to file
    char filename2[150];
    sprintf(filename2, "%s_per_row.txt", outputPrefix);
    writeMatrix(filename2, C2, resultRows, resultCols);
    
    //-------------------------------------------------------------------//
    // Method 3: One thread per element.
    //-------------------------------------------------------------------//
    int totalThreads = resultRows * resultCols;  // Total threads equals number of elements
    pthread_t *threads3 = malloc(totalThreads * sizeof(pthread_t)); // Dynamic array for thread identifiers
    gettimeofday(&start, NULL);  // Start timing for Method 3
    int t = 0;
    // Create a thread for each element in the output matrix
    for (int i = 0; i < resultRows; i++) {
        for (int j = 0; j < resultCols; j++) {
            threadElemArg *arg = (threadElemArg *) malloc(sizeof(threadElemArg));
            arg->row = i;
            arg->col = j;
            if (pthread_create(&threads3[t], NULL, multiplyElement, arg)) {
                printf("Error creating thread for element (%d,%d) in method 3\n", i, j);
                exit(1);
            }
            t++;
        }
    }
    // Wait for all element threads to finish
    for (t = 0; t < totalThreads; t++) {
        if (pthread_join(threads3[t], NULL)) {
            printf("Error joining thread %d in method 3\n", t);
            exit(1);
        }
    }
    gettimeofday(&stop, NULL);  // End timing for Method 3
    seconds = stop.tv_sec - start.tv_sec;
    microseconds = stop.tv_usec - start.tv_usec;
    // Print the total number of threads and execution time for Method 3
    printf("Method 3 (per element): Threads = %d, Time taken: %ld seconds and %ld microseconds\n", totalThreads, seconds, microseconds);
    
    // Write result matrix C3 to file
    char filename3[150];
    sprintf(filename3, "%s_per_element.txt", outputPrefix);
    writeMatrix(filename3, C3, resultRows, resultCols);
    
    //-------------------------------------------------------------------//
    // Cleanup: Free all allocated memory.
    //-------------------------------------------------------------------//
    freeMatrix(A, A_rows);
    freeMatrix(B, B_rows);
    freeMatrix(C1, resultRows);
    freeMatrix(C2, resultRows);
    freeMatrix(C3, resultRows);
    free(threads3);  // Free the dynamic array for method 3 threads
    
    return 0;  // End of program
}
