#include "process.hpp"

#include <cstdio>
#include <mpi.h>
#include <random>

extern "C" {
#include "mpi_error_check.h"
}

namespace chem {
  void score(molecule& mol) {
    // "compute" the score
    auto generator = std::mt19937{mol.id};
    auto dist             = std::uniform_real_distribution<float>{0.0f, 10.0f};
    mol.score             = dist(generator);

    // log who performed the computation
    int world_rank    = 0;
    const int rc_rank = MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    exit_on_fail(rc_rank);
    printf("CMP [MPI %d] Computed molecule with ID %d (score: %3.2f)\n", world_rank, mol.id, mol.score);
  }
} // namespace chem
