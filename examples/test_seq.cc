
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct EmptyMsg : vt::Message {
  EmptyMsg() : Message() { }
};

#define PRINT_SEQUENCE_ON 1

#if PRINT_SEQUENCE_ON
#define PRINT_SEQUENCE(fmt, arg...)                                     \
  do {                                                                  \
    printf(                                                             \
      "%d: seq_id=%d: " fmt, theContext->get_node(),                   \
      theSeq->get_current_seq(), ##arg                                 \
    );                                                                  \
  } while (0);
#else
#define PRINT_SEQUENCE
#endif

sequence_register_handler(EmptyMsg, action1);

static void my_seq(SeqType const& seq_id) {
  PRINT_SEQUENCE("my_seq: executing sequence\n");

  theSeq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    PRINT_SEQUENCE("action1 WAIT-1 triggered\n");
  });

  theSeq->sequenced_block([]{
    PRINT_SEQUENCE("action1 sequenced_block triggered\n");
    auto const& my_node = theContext->get_node();
    theMsg->send_msg<EmptyMsg, action1>(
      my_node, make_shared_message<EmptyMsg>(), 20
    );
  });

  theSeq->sequenced(seq_id, [=]{
    theSeq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-2 triggered\n");
    });

    theSeq->wait<EmptyMsg, action1>(30, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-3 triggered\n");
    });
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = theContext->get_node();
  num_nodes = theContext->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->next_seq();
    theSeq->sequenced(seq_id, my_seq);

    theMsg->send_msg<EmptyMsg, action1>(1, make_shared_message<EmptyMsg>(), 30);
    theMsg->send_msg<EmptyMsg, action1>(1, make_shared_message<EmptyMsg>(), 10);

    theMsg->send_msg<EmptyMsg, action1>(0, make_shared_message<EmptyMsg>(), 30);
    theMsg->send_msg<EmptyMsg, action1>(0, make_shared_message<EmptyMsg>(), 10);
  } else if (my_node == 1) {
    SeqType const& seq_id2 = theSeq->next_seq();
    theSeq->sequenced(seq_id2, my_seq);
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
