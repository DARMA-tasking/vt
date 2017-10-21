
#include <cassert>
#include <cstdint>

#include "transport.h"

#define DEBUG_PING_PONG 0
#define REUSE_MESSAGE_PING_PONG 0

using namespace vt;

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;

static int64_t num_pings = 1024;

static constexpr NodeType const ping_node = 0;
static constexpr NodeType const pong_node = 1;

static double startTime = 0.0;

template <int64_t num_bytes>
struct PingMsg : ShortMessage {
  int64_t count = 0;
  std::array<char, num_bytes> payload;

  PingMsg() : ShortMessage() { }
  PingMsg(int64_t const in_count) : ShortMessage(), count(in_count) { }
};

template <int64_t num_bytes>
struct FinishedPingMsg : ShortMessage {
  int64_t prev_bytes = 0;

  FinishedPingMsg(int64_t const in_prev_bytes)
    : ShortMessage(), prev_bytes(in_prev_bytes)
  { }
};

template <int64_t num_bytes>
static void pingPong(PingMsg<num_bytes>* in_msg);

template <int64_t num_bytes>
static void finishedPing(FinishedPingMsg<num_bytes>* msg);

static void printTiming(int64_t const& num_bytes) {
  double const total = MPI_Wtime() - startTime;

  startTime = MPI_Wtime();

  printf(
    "%d: Finished num_pings=%lld, bytes=%lld, total time=%f, time/msg=%f\n",
    theContext()->getNode(), num_pings, num_bytes, total, total/num_pings
  );
}

template <int64_t num_bytes>
static void finishedPing(FinishedPingMsg<num_bytes>* msg) {
  printTiming(num_bytes);

  if (num_bytes != max_bytes) {
    auto msg = new PingMsg<num_bytes * 2>();
    theMsg()->sendMsg<PingMsg<num_bytes * 2>, pingPong>(
      pong_node, msg, [=]{ delete msg; }
    );
  }
}

template <>
void finishedPing<max_bytes>(FinishedPingMsg<max_bytes>* msg) {
  printTiming(max_bytes);
}

template <int64_t num_bytes>
static void pingPong(PingMsg<num_bytes>* in_msg) {
  auto const& cnt = in_msg->count;

  #if DEBUG_PING_PONG
    printf(
      "%d: pingPong: cnt=%lld, bytes=%lld\n",
      theContext()->getNode(), cnt, num_bytes
    );
  #endif

  #if REUSE_MESSAGE_PING_PONG
    in_msg->count++;
    messageRef(in_msg);
  #endif

  if (cnt >= num_pings) {
    auto msg = new FinishedPingMsg<num_bytes>(num_bytes);
    theMsg()->sendMsg<FinishedPingMsg<num_bytes>, finishedPing>(
      0, msg, [=]{ delete msg; }
    );
  } else {
    NodeType const next =
      theContext()->getNode() == ping_node ? pong_node : ping_node;
    #if REUSE_MESSAGE_PING_PONG
      // @todo: fix this memory allocation problem
      theMsg()->sendMsg<PingMsg<num_bytes>, pingPong>(
        next, in_msg, [=]{ /*delete in_msg;*/ }
      );
    #else
      auto m = new PingMsg<num_bytes>(cnt + 1);
      theMsg()->sendMsg<PingMsg<num_bytes>, pingPong>(next, m, [=]{ delete m; });
    #endif
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (argc > 1) {
    num_pings = atoi(argv[1]);
  }

  startTime = MPI_Wtime();

  if (my_node == 0) {
    auto m = new PingMsg<min_bytes>();
    theMsg()->sendMsg<PingMsg<min_bytes>, pingPong>(pong_node, m, [=]{ delete m; });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
