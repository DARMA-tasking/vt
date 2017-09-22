
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
      "%d: seq_id=%d: " fmt, theContext->getNode(),                     \
      theSeq->getCurrentSeq(), ##arg                                    \
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

      auto const& nd = my_node == 0 ? 1 : 0;
      // send messages for the parallel region following
      theMsg->sendMsg<EmptyMsg, action1>(nd, makeSharedMessage<EmptyMsg>(), 229);
      //theMsg->sendMsg<EmptyMsg, action1>(nd, makeSharedMessage<EmptyMsg>(), 129);

      theMsg->sendMsg<EmptyMsg, action1>(nd, makeSharedMessage<EmptyMsg>(), 900);
    });
  });

  theSeq->parallel(seq_id, []{
    theSeq->wait<EmptyMsg, action1>(129, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-4 parallel triggered\n");
    });
  }, []{
    theSeq->wait<EmptyMsg, action1>(229, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-5 parallel triggered\n");
    });
  });

  theSeq->sequencedBlock([]{
    theSeq->wait<EmptyMsg, action1>(900, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-6 FINAL triggered\n");
    });
  });
}

static void simpleSeq(SeqType const& seq_id) {
  PRINT_SEQUENCE("simpleSeq: executing sequence: seq_id=%d\n", seq_id);

  theSeq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    auto const& seq_id = theSeq->getCurrentSeq();
    PRINT_SEQUENCE("simpleSeq: seq_id=%d: action1 WAIT-1 triggered\n", seq_id);
  });

  theSeq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
    auto const& seq_id = theSeq->getCurrentSeq();
    PRINT_SEQUENCE("simpleSeq: seq_id=%d: action1 WAIT-2 triggered\n", seq_id);
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext->getNode();
  num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, simpleSeq);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 20);
  }

  // if (my_node == 0) {
  //   SeqType const& seq_id = theSeq->nextSeq();
  //   theSeq->sequenced(seq_id, mySeq);

  //   theMsg->sendMsg<EmptyMsg, action1>(1, makeSharedMessage<EmptyMsg>(), 30);
  //   theMsg->sendMsg<EmptyMsg, action1>(1, makeSharedMessage<EmptyMsg>(), 10);

  //   theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 30);
  //   theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10);
  // } else if (my_node == 1) {
  //   SeqType const& seq_id2 = theSeq->nextSeq();
  //   theSeq->sequenced(seq_id2, mySeq);
  // }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
