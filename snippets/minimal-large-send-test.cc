
#include <mpi.h>

#include <vector>
#include <memory>

std::vector<std::unique_ptr<char[]>> data;
std::vector<MPI_Request> reqs;

void send(int tag) {
  auto size = 1 << 26;
  data.emplace_back(std::make_unique<char[]>(size));
  reqs.emplace_back(MPI_REQUEST_NULL);
  auto& req = reqs.back();
  printf("sending data: size=%d\n", size);
  MPI_Isend(&data.back()[0], size, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &req);
}

void recv(int source, int tag, int bytes) {
  data.emplace_back(std::make_unique<char[]>(bytes));
  reqs.emplace_back(MPI_REQUEST_NULL);
  auto& req = reqs.back();
  printf("recving data: size=%d, tag=%d, source=%d\n", bytes, tag, source);
  MPI_Irecv(&data.back()[0], bytes, MPI_BYTE, source, tag, MPI_COMM_WORLD, &req);
}

int probe() {
  int flag = 0;
  MPI_Status stat;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
  if (flag == 1) {
    int bytes = 0;
    MPI_Get_count(&stat, MPI_BYTE, &bytes);
    recv(stat.MPI_SOURCE, stat.MPI_TAG, bytes);
  }
  return flag;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  printf("rank=%d\n",rank);

  int num = 10;

  for (int i = 0; i < num; i++) {
    if (rank == 0) {
      send(10+i);
    } else {
      while (reqs.size() < num) {
        probe();
      }
    }
  }

  std::vector<MPI_Status> stats;
  stats.resize(reqs.size());
  MPI_Waitall(reqs.size(), &reqs[0], &stats[0]);

  MPI_Finalize();

  return 0;
};
