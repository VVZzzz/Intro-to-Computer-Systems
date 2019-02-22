/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  /*
   */
  int col;
  int row;
  int c;
  int r;
  int buffer[8];  // buffer[0]=temp,buffer[1]=d
  if (N == 32) {
    // block size = 8 * 8
    for (row = 0; row < N; row += 8) {
      for (col = 0; col < M; col += 8) {
        for (r = row; r < row + 8; r++) {
          for (c = col; c < col + 8; c++) {
            if (r != c)
              B[c][r] = A[r][c];
            else {
              buffer[0] = A[r][c];
              buffer[1] = r;
            }
          }
          if (row == col) B[buffer[1]][buffer[1]] = buffer[0];
        }
      }
    }
  } else if (N == 64) {
    // block size is 8 * 8;
    for (row = 0; row < N; row += 8) {
      for (col = 0; col < M; col += 8) {
        for (r = row; r < row + 4; r++) {
          for (c = 0; c < 8; c++) buffer[c] = A[r][col + c];
          for (c = 0; c < 4; c++) {
            B[col + c][r] = buffer[c];
            B[col + c][r + 4] = buffer[c + 4];
          }
        }

        for (c = col; c < col + 4; c++) {
          for (r = 0; r < 4; r++) buffer[r] = B[c][row + 4 + r];
          for (r = 4; r < 8; r++) B[c][row + r] = A[row + r][c];
          for (r = 0; r < 4; r++) B[c + 4][row + r] = buffer[r];
          for (r = 4; r < 8; r++) B[c + 4][row + r] = A[row + r][c + 4];
        }
      }
    }
  } else {
    // M==61 N==67 block size is 8*8.
    // is the same as N==32
    for (row = 0; row < 67; row += 16) {
      for (col = 0; col < 61; col += 16) {
        for (r = row; r < row + 16 && r < 67; r++) {
          for (c = col; c < col + 16 && c < 61; c++) {
            if (r != c)
              B[c][r] = A[r][c];
            else {
              buffer[0] = A[r][c];
              buffer[1] = r;
            }
          }
          if (row == col) B[buffer[1]][buffer[1]] = buffer[0];
        }
      }
    }
  }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
  int i, j;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; ++j) {
      if (A[i][j] != B[j][i]) {
        return 0;
      }
    }
  }
  return 1;
}
