
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
      "%d: seq_id=%d: " fmt, theContext->getNode(),                   \
      theSeq->get_current_seq(), ##arg                                 \
    );                                                                  \
  } while (0);
#else
#define PRINT_SEQUENCE
#endif

SEQUENCE_REGISTER_HANDLER(EmptyMsg, action1);

static void mySeq(SeqType const& seq_id) {
  PRINT_SEQUENCE("mySeq: executing sequence\n");

  theSeq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    PRINT_SEQUENCE("action1 WAIT-1 triggered\n");
  });

  theSeq->sequencedBlock([]{
    PRINT_SEQUENCE("action1 sequenced_block triggered\n");
    auto const& my_node = theContext->getNode();
    theMsg->sendMsg<EmptyMsg, action1>(
      my_node, makeSharedMessage<EmptyMsg>(), 20
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
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  my_node = theContext->getNode();
  num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, mySeq);

    theMsg->sendMsg<EmptyMsg, action1>(1, makeSharedMessage<EmptyMsg>(), 30);
    theMsg->sendMsg<EmptyMsg, action1>(1, makeSharedMessage<EmptyMsg>(), 10);

    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 30);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10);
  } else if (my_node == 1) {
    SeqType const& seq_id2 = theSeq->nextSeq();
    theSeq->sequenced(seq_id2, mySeq);
  }

  while (1) {
    runScheduler();
  }

  return 0;
}
