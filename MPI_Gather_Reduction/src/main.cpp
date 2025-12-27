#include "data.hpp"
#include "process.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <sstream>
#include <cmath>

extern "C" {
#include "mpi_error_check.h"
}

const int tag_reduce = 1;

int main(int argc, char* argv[]) {
  // initialize the MPI environment
  int provided_thread_level;
  const int rc_init = MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided_thread_level);
  exit_on_fail(rc_init);
  if (provided_thread_level < MPI_THREAD_SINGLE) {
    std::cerr << "Minimum MPI level not satisfied ..." << std::endl;
    std::cerr << "Out of curiosity, which implementation are you using?" << std::endl;
    return EXIT_FAILURE;
  }

  //==---------------------------------------------------------------------------------------------------==//
  // DATA GENERATION PHASE
  //==---------------------------------------------------------------------------------------------------==//
  // This phase aim at generating the dataset to compute. It mimicks the operation of reading molecules from
  // a file and parse them. Leave this section as it is, at the end the process that happen to have rank 0
  // will have all the initialized data.
  // Please, note that the number of molecules comes from a command line argument, so it is known at runtime
  // but YOU CAN ASSUME THAT THEY ARE A MULTIPLE OF THE NUMBER PROCESSSES

  int world_rank    = 0;
  const int rc_rank = MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  exit_on_fail(rc_rank);
  auto data = std::vector<chem::molecule>{};
  if (world_rank == 0) {
    if (argc < 2) {
      std::cerr << "Error: illegal parameter number!" << std::endl;
      std::cout << "Usage:" << std::endl;
      std::cout << "\t" << argv[0] << " num_data" << std::endl;
      return EXIT_FAILURE;
    }
    auto parser   = std::istringstream(argv[1]);
    auto num_data = std::uint32_t{0};
    parser >> num_data;
    if (parser.fail()) {
      std::cerr << "Error: unable to understand the number \"" << argv[1] << '"' << std::endl;
      return EXIT_FAILURE;
    }
    data = chem::generate_data(num_data);
  }

  //==---------------------------------------------------------------------------------------------------==//
  // COMPUTATION PHASE
  //==---------------------------------------------------------------------------------------------------==//
  // You should distribute the computation across all the processes that the user launched. In this skeleton
  // application the rank 0 has all the data and computes all the data. You can use this code to generate the
  // reference data and check if they remain the same while changing the number of processes

  MPI_Datatype MPI_MOLECULE = chem::create_molecule_MPI_type();
  //data is the array which keeps all the molecules -> need to split among all the available processes
  int world_size    = 0;
  const int rc_size = MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  exit_on_fail(rc_size);

  //Process with rank 0 calculates the number of molecules
  //The number of molecules is then broadcasted and each process has its value in recvcount
  int recvcount = 0;

  if (world_rank == 0)
    recvcount = data.size() / world_size;

  const int rc_bcast = MPI_Bcast(&recvcount, 1, MPI_INT, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_bcast);

  //Now, data are scattered among all the available processes (0 included)
  chem::molecule* sendbuf = nullptr;
  if (world_rank == 0)
    sendbuf = data.data();

  auto received = std::vector<chem::molecule>(recvcount); //initialized with the correct value
  chem::molecule* recvbuf  = received.data();

  const int rc_scatter = MPI_Scatter(sendbuf, recvcount, MPI_MOLECULE, recvbuf, recvcount, MPI_MOLECULE, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_scatter);
  //Each process has received its elements and now proceeds to score the molecules
  for (auto& molecule: received) { chem::score(molecule); }

  //==---------------------------------------------------------------------------------------------------==//
  // REDUCTION PHASE
  //==---------------------------------------------------------------------------------------------------==//
  // Each process should have the scores of an input subset. The goal of this phase is to perform a reduction
  // and collect in one process the global optimal 1% of molecules. In this skeleton application is trivial
  // since the process with rank 0 has already all the data.

  /* Each process performs the partial sort on its own portion of the data.
  Parallel reduction:
    Divide the processes in groups of two
    Each second process sends his top 1% data to the other
    The receiving one performs the partial sort on the new data, the other terminates

    Divide the remaining processes in groups of two...

    The number of steps in the reduction will be lg(world_size)
  */

  const auto num_output = 
    std::max(static_cast<std::size_t>(static_cast<float>(recvcount*world_size) * 0.01), std::size_t{1});
  
  int num_steps = static_cast<int>(std::ceil(std::log2(world_size)));
  for(int step = 1; step <= num_steps; step++){
    int div = 1 << step; //divisor for the color of the groups
    int color = world_rank/div;

    //Sorts the vector in the way that the first num_output molecules contain the top 1%
    std::partial_sort(std::begin(received),
                      std::begin(received) + num_output,
                      std::end(received)); // lower indexes better molecules
    //Resizes data to exclude the elements not in the 1%
    if (received.size() > num_output)
      received.resize(num_output);

    MPI_Comm newcomm;
    const int rc_split = MPI_Comm_split(MPI_COMM_WORLD, color, world_rank % 2, &newcomm);
    exit_on_fail(rc_split);

    int step_rank = 0;
    const int rc_rank = MPI_Comm_rank(newcomm, &step_rank);
    exit_on_fail(rc_rank);

    int step_size = 0;
    const int rc_size = MPI_Comm_size(newcomm, &step_size);
    exit_on_fail(rc_size);

    //Assign a partner to each process
    int partner = 0;
    if(step_rank % 2 == 0){
      partner = step_rank + 1;
    }else{
      partner = step_rank - 1;
    }

    if(partner >= step_size) partner = MPI_PROC_NULL;

    if(step_rank == 0){
      //vector to store the received data
      auto new_data = std::vector<chem::molecule>(num_output); 
      chem::molecule* recvbuf  = new_data.data();
      //receive the data from the other group member
      const int rc_recv = MPI_Recv(recvbuf, num_output, MPI_MOLECULE, partner, tag_reduce, newcomm, MPI_STATUS_IGNORE);
      exit_on_fail(rc_recv);
      //concatenate the vectors to store all the data
      received.insert(received.end(), new_data.begin(), new_data.end());
    }else{
      const int rc_send = MPI_Send(recvbuf, num_output, MPI_MOLECULE, partner, tag_reduce, newcomm);
      exit_on_fail(rc_send);
    }

    const int rc_free = MPI_Comm_free(&newcomm);
    exit_on_fail(rc_free);
  }

  if(world_rank == 0){
    //Sorts the vector in the way that the first num_output molecules contain the top 1%
    std::partial_sort(std::begin(received),
                      std::begin(received) + num_output,
                      std::end(received)); // lower indexes better molecules
    //Resizes data to exclude the elements not in the 1%
    if (received.size() > num_output)
      received.resize(num_output);
  }

  //==---------------------------------------------------------------------------------------------------==//
  // OUTPUT PHASE
  //==---------------------------------------------------------------------------------------------------==//
  // The process with the output must call this function to display the data. The validator will use those
  // lines to check whether the application provides the correct answer.
  if(world_rank == 0) print_data(received);

  // clear the MPI environment
  const int rc_finalize = MPI_Finalize();
  exit_on_fail(rc_finalize);

  // if we reach this point, everything is fine
  return EXIT_SUCCESS;
}
