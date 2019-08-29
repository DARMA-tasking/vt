

#include <vt/transport.h>

#include <cstdlib>
#include <array>

struct PingMsg : vt::Message {
  PingMsg() = default;
  explicit PingMsg(int64_t size) {
    payload_.resize(size);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | payload_;
  }

  std::vector<char> payload_;
};

static constexpr int64_t const max_bytes = 0x1000000;
static int pings = 64;
static bool is_done = false;

static void done(PingMsg* msg) {
  is_done = true;
}

static void handler(PingMsg*) {
  static int count = 0;
  count++;
  if (count == pings) {
    auto msg = vt::makeMessage<PingMsg>(1);
    vt::theMsg()->sendMsgAuto<PingMsg,done>(0, msg.get());
    count = 0;
  }
}

template <int64_t bytes>
void sender() {
  auto start = vt::timing::Timing::getCurrentTime();
  for (int i = 0; i < pings; i++) {
    auto msg = vt::makeMessage<PingMsg>(bytes);
    vt::theMsg()->sendMsgAuto<PingMsg,handler>(1, msg.get());
  }
  while (not is_done) vt::runScheduler();
  is_done = false;
  auto time = (vt::timing::Timing::getCurrentTime() - start) / pings;
  auto Mb = static_cast<double>(bytes) / 1024.0 / 1024.0;
  fmt::print("{:<8} {:<16} 0x{:<10x} {:<22} {:<22}\n", pings, bytes, bytes, Mb, time);
}

template <int64_t bytes>
void send() {
  sender<bytes>();
  send<bytes << 1>();
}

template <>
void send<max_bytes>() {
  sender<max_bytes>();
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  auto const this_node = vt::theContext()->getNode();
  auto const num_nodes = vt::theContext()->getNumNodes();

  vtAssertExpr(argc == 2);

  pings = atoi(argv[1]);

  if (this_node == 0) {
    fmt::print(
      "{:<8} {:<16} 0x{:<10} {:<22} {:<22}\n",
      "Pings", "Bytes", "Bytes", "Mb", "Time per"
    );
    send<0x1>();
  }

  vt::finalize();
}
