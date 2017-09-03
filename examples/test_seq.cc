
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

struct EmptyMsg : runtime::Message {
  EmptyMsg() : Message() { }
};

#define PRINT_SEQUENCE_ON 1

#if PRINT_SEQUENCE_ON
#define PRINT_SEQUENCE(fmt, arg...)                                     \
  do {                                                                  \
    printf(                                                             \
      "%d: seq_id=%d: " fmt, the_context->get_node(),                   \
      the_seq->get_current_seq(), ##arg                                 \
    );                                                                  \
  } while (0);
#else
#define PRINT_SEQUENCE
#endif

sequence_register_handler(EmptyMsg, action1);

static void my_seq(seq_t const& seq_id) {
  PRINT_SEQUENCE("my_seq: executing sequence\n");

  the_seq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    PRINT_SEQUENCE("action1 WAIT-1 triggered\n");

    auto const& my_node = the_context->get_node();
    the_msg->send_msg<EmptyMsg, action1>(
      my_node, make_shared_message<EmptyMsg>(), 20
    );
  });

  the_seq->sequenced(seq_id, [=]{
    the_seq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-2 triggered\n");
    });

    the_seq->wait<EmptyMsg, action1>(30, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-3 triggered\n");
    });
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    seq_t const& seq_id = the_seq->next_seq();
    the_seq->sequenced(seq_id, my_seq);

    the_msg->send_msg<EmptyMsg, action1>(1, make_shared_message<EmptyMsg>(), 30);
    the_msg->send_msg<EmptyMsg, action1>(1, make_shared_message<EmptyMsg>(), 10);

    the_msg->send_msg<EmptyMsg, action1>(0, make_shared_message<EmptyMsg>(), 30);
    the_msg->send_msg<EmptyMsg, action1>(0, make_shared_message<EmptyMsg>(), 10);
  } else if (my_node == 1) {
    seq_t const& seq_id2 = the_seq->next_seq();
    the_seq->sequenced(seq_id2, my_seq);
  }

  while (1) {
    the_msg->scheduler();
    the_seq->scheduler();
  }

  return 0;
}
