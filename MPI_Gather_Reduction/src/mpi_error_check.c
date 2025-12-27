#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "mpi_error_check.h"

void exit_on_fail(const int return_code) {
  if (return_code != MPI_SUCCESS) {
    print_mpi_error(return_code, stderr);
    abort();
  }
}

void print_mpi_error(const int return_code, FILE* const output_file) {
  assert(return_code != MPI_SUCCESS);
  assert(output_file != NULL);

  // get the error class (which should be standard across the MPI implementations)
  int error_class = MPI_ERR_UNKNOWN;
  const int rc_class = MPI_Error_class(return_code, &error_class);
  if (rc_class != MPI_SUCCESS) {
    error_class = MPI_ERR_UNKNOWN;
  }

  // get a string that describes the error
  char error_explanation[MPI_MAX_ERROR_STRING];
  int explanation_len = 0;
  const int rc_string = MPI_Error_string(return_code, error_explanation, &explanation_len);
  if (rc_string == MPI_SUCCESS) {
    assert(explanation_len < MPI_MAX_ERROR_STRING);
    error_explanation[explanation_len] = '\0'; // to be sure to have a valid c-string
  } else {
    strncpy(error_explanation, "Explanation not available", MPI_MAX_ERROR_STRING);
  }

  // notify the user
  fprintf(output_file, "MPI error %i: \"%s\"\n", error_class, error_explanation);
}