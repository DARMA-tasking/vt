
#include <mpi.h>

#include <cstdio>
#include <cassert>

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank = 0, nranks = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nranks);

  printf("rank=%d\n",rank);

  int* data_base_ = nullptr;
  MPI_Win data_window_;

  std::size_t const num = 10;
  int ret = 0;
  ret = MPI_Alloc_mem(num * sizeof(int), MPI_INFO_NULL, &data_base_);
  assert(ret == 0);
  ret = MPI_Win_create(
    data_base_, num * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,
    &data_window_
  );
  assert(ret == 0);

  MPI_Barrier(MPI_COMM_WORLD);
  for (std::size_t i = 0; i < num; i++) {
    data_base_[i] = i+1;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  printf("rank=%d: update data\n", rank);

  int* ptr = new int[10];
  auto nrank = (rank + 1) % nranks;
  ret = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, nrank, 0, data_window_);
  assert(ret == 0);
  MPI_Get(ptr, num, MPI_INT32_T, nrank, 0, num, MPI_INT32_T, data_window_);
  ret = MPI_Win_unlock(nrank, data_window_);
  assert(ret == 0);

  printf("rank=%d: get data\n", rank);

  MPI_Finalize();

  return 0;
};
