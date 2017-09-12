
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static handler_t my_reinstate_fn = uninitialized_handler;
static tag_t const first_recv_tag = 10;
static tag_t const last_recv_tag = 15;
static tag_t cur_iter = first_recv_tag;
static int count = 0;

struct TestMsg : runtime::Message {
  NodeType from;
  handler_t callback_han;

  TestMsg(NodeType const& in_from, handler_t const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void process_iter_msgs(runtime::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  auto const& first_tag = envelope_get_tag(msg.env);

  count++;

  printf(
    "%d: process iteration node %d: count=%d, tag=%d, iteration=%d\n",
    the_context->get_node(), msg.from, count, first_tag, cur_iter
  );

  assert(first_tag == cur_iter);

  // received all for this iteration
  if (count == the_context->get_num_nodes() - 1) {
    cur_iter++;
    count = 0;

    auto const& first_han = the_msg->get_current_handler();
    the_msg->unregister_handler_fn(first_han, cur_iter-1);
    the_msg->register_handler_fn(first_han, process_iter_msgs, cur_iter);

    printf(
      "%d: updating to NEXT iteration node %d: count=%d, cur_iter=%d\n",
      the_context->get_node(), msg.from, count, cur_iter
    );
  }
}

static void my_col_fn(TestMsg* msg) {
  auto const& my_node = the_context->get_node();

  printf(
    "%d: my_col_fn from=%d, callback=%d: tag=%d, sending, tag=[%d,%d]\n",
    my_node, msg->from, msg->callback_han, first_recv_tag, first_recv_tag, last_recv_tag
  );

  for (auto i = first_recv_tag; i < last_recv_tag; i++) {
    TestMsg* new_msg = make_shared_message<TestMsg>(my_node, uninitialized_handler);
    the_msg->send_msg(msg->callback_han, new_msg, i);
    message_deref(new_msg);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  handler_t const callback = the_msg->register_new_handler(
    process_iter_msgs, cur_iter
  );

  auto const& my_node = the_context->get_node();
  auto const& num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    TestMsg* msg = make_shared_message<TestMsg>(my_node, callback);
    the_msg->broadcast_msg<TestMsg, my_col_fn>(msg);
    message_deref(msg);
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
