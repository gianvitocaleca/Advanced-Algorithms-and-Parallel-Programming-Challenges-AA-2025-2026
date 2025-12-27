#include "data.hpp"

#include <cstdint>
#include <cstdio>
#include <mpi.h>
#include <random>

extern "C" {
#include "mpi_error_check.h"
}

namespace chem {

  std::vector<molecule> generate_data(const std::size_t num_molecule) {
    // initialize a source of randomness
    static auto generator = std::mt19937{1234};
    auto dist             = std::uniform_int_distribution<std::uint32_t>{1, 5};

    // generate the molecule IDs
    std::vector<molecule> molecules(num_molecule); // allocate memory
    auto counter = dist(generator);
    for (auto& mol: molecules) {
      mol.id = counter;
      counter += dist(generator);
    }
    return molecules;
  }

  void print_data(const std::vector<molecule>& data) {
    for (const auto molecule: data) { printf("OUT Candidate ID %d\n", molecule.id); }
  }

  MPI_Datatype create_molecule_MPI_type() {
    // describe the struct fields
    const int count           = 2;
    int block_lengths[count]  = {1, 1};
    MPI_Aint offsets[count]   = {offsetof(molecule, id), offsetof(molecule, score)};
    MPI_Datatype types[count] = {MPI_UINT32_T, MPI_FLOAT};

    // create the actual MPI type
    MPI_Datatype dtype;
    MPI_Type_create_struct(count, block_lengths, offsets, types, &dtype);
    MPI_Type_commit(&dtype);
    return dtype;
  }

} // namespace chem
