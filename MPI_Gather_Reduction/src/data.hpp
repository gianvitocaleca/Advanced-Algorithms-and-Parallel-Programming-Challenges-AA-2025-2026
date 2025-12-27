#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <mpi.h>
#include <vector>

namespace chem {

  struct molecule {
    std::uint32_t id = 0;    // "name" of the molecule
    float score      = 0.0f; // the computed value by the program
  };

  // utility function to initialize the data
  std::vector<molecule> generate_data(const std::size_t num_molecule);

  // utility function to write the data on the stdout
  void print_data(const std::vector<molecule>& data);

  // utility function to create the molecule data type
  MPI_Datatype create_molecule_MPI_type();

  // comparators utilities
  inline bool operator<(molecule const& a, molecule const& b) {
    return std::abs(a.score - b.score) > std::numeric_limits<float>::epsilon() ? a.score > b.score
                                                                               : a.id > b.id;
  }

} // namespace chem
