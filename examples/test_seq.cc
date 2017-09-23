
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

  theSeq->sequenced([]{
    PRINT_SEQUENCE("action1 sequenced_block triggered\n");
    auto const& my_node = theContext->getNode();
    theMsg->sendMsg<EmptyMsg, action1>(
      my_node, makeSharedMessage<EmptyMsg>(), 20
    );
  });

  theSeq->sequenced(seq_id, [=]{
    PRINT_SEQUENCE("SEQUENCED: (begin) unraveling child closure triggered\n\n");

    theSeq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-2 triggered\n");
    });

    theSeq->wait<EmptyMsg, action1>(30, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-3 triggered\n");

      auto const& nd = my_node == 0 ? 1 : 0;
      // send messages for the parallel region following
      //theMsg->sendMsg<EmptyMsg, action1>(nd, makeSharedMessage<EmptyMsg>(), 229);
      //theMsg->sendMsg<EmptyMsg, action1>(nd, makeSharedMessage<EmptyMsg>(), 129);

      theMsg->sendMsg<EmptyMsg, action1>(nd, makeSharedMessage<EmptyMsg>(), 900);
    });

    PRINT_SEQUENCE("SEQUENCED: (end) unraveling child closure triggered\n\n");
  });

  theSeq->sequenced([]{
    theSeq->wait<EmptyMsg, action1>(900, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-6 FINAL triggered\n");
    });
  });
}

static void mySeqParallel(SeqType const& seq_id) {
  PRINT_SEQUENCE("mySeqParallel: executing sequence\n");

  theSeq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    PRINT_SEQUENCE("action1 w1 triggered\n");
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 20);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 30);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 40);
  });

  theSeq->parallel(seq_id, []{
    theSeq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 w2-par-1 parallel triggered\n");
    });
  }, []{
    theSeq->wait<EmptyMsg, action1>(30, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 w2-par-2 parallel triggered\n");
    });
  });

  theSeq->sequenced([]{
    theSeq->wait<EmptyMsg, action1>(40, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 w3 FINAL triggered\n");
    });
  });
}

static void mySeqSingleNode(SeqType const& seq_id) {
  PRINT_SEQUENCE("mySeqSingleNode: executing sequence\n");

  theSeq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    PRINT_SEQUENCE("action1 WAIT-1 triggered\n");
  });

  theSeq->sequenced([]{
    PRINT_SEQUENCE("action1 sequenced_block triggered\n");
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 20);
  });

  theSeq->sequenced(seq_id, [=]{
    PRINT_SEQUENCE("SEQUENCED: (begin) unraveling child closure triggered\n");

    theSeq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-2 triggered\n");
    });

    theSeq->wait<EmptyMsg, action1>(30, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-3 triggered\n");
      theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 900);
    });

    PRINT_SEQUENCE("SEQUENCED: (end) unraveling child closure triggered\n");
  });

  theSeq->sequenced([]{
    PRINT_SEQUENCE("SEQUENCED: (begin) unraveling FINAL child closure triggered\n");

    theSeq->wait<EmptyMsg, action1>(900, [](EmptyMsg* msg){
      PRINT_SEQUENCE("action1 WAIT-6 FINAL triggered\n");
    });
  });
}

static void simpleSeq(SeqType const& seq_id) {
  PRINT_SEQUENCE("simpleSeq: executing sequence: seq_id=%d\n", seq_id);

  theSeq->wait<EmptyMsg, action1>(10, [](EmptyMsg* msg){
    auto const& seq_id = theSeq->getCurrentSeq();
    PRINT_SEQUENCE("simpleSeq: seq=%d: w1 triggered: msg=%p\n", seq_id, msg);
  });

  theSeq->sequenced([]{
    PRINT_SEQUENCE("SEQUENCED: (begin) unraveling child closure\n");
    theSeq->wait<EmptyMsg, action1>(100, [](EmptyMsg* msg){
      PRINT_SEQUENCE("simpleSeq: w100 inner triggered\n");
    });
    theSeq->wait<EmptyMsg, action1>(200, [](EmptyMsg* msg){
      PRINT_SEQUENCE("simpleSeq: w200 inner triggered\n");
    });
    PRINT_SEQUENCE("SEQUENCED: (end) unraveling child closure\n");
  });

  theSeq->wait<EmptyMsg, action1>(20, [](EmptyMsg* msg){
    auto const& seq_id = theSeq->getCurrentSeq();
    PRINT_SEQUENCE("simpleSeq: seq=%d: w2 triggered: msg=%p\n", seq_id, msg);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 30);
  });

  theSeq->wait<EmptyMsg, action1>(30, [](EmptyMsg* msg){
    auto const& seq_id = theSeq->getCurrentSeq();
    PRINT_SEQUENCE("simpleSeq: seq=%d: w3 triggered\n", seq_id);
  });

  theSeq->sequenced([]{
    PRINT_SEQUENCE("SEQUENCED: (begin) unraveling child closure 2\n");
    theSeq->wait<EmptyMsg, action1>(1000, [](EmptyMsg* msg){
      PRINT_SEQUENCE("simpleSeq: w1000 inner triggered\n");
      theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 40);
    });

    theSeq->sequenced([]{
      PRINT_SEQUENCE("SEQUENCED: (begin) unraveling child closure 2a\n");
      theSeq->wait<EmptyMsg, action1>(10000, [](EmptyMsg* msg){
        PRINT_SEQUENCE("simpleSeq: w10000 inner-inner triggered\n");
      });
      theSeq->wait<EmptyMsg, action1>(20000, [](EmptyMsg* msg){
        PRINT_SEQUENCE("simpleSeq: w20000 inner-inner triggered\n");
      });
      PRINT_SEQUENCE("SEQUENCED: (end) unraveling child closure 2a\n");
    });

    theSeq->wait<EmptyMsg, action1>(2000, [](EmptyMsg* msg){
      PRINT_SEQUENCE("simpleSeq: w2000 inner triggered\n");
    });
    PRINT_SEQUENCE("SEQUENCED: (end) unraveling child closure 2\n");
  });

  theSeq->wait<EmptyMsg, action1>(40, [](EmptyMsg* msg){
    auto const& seq_id = theSeq->getCurrentSeq();
    PRINT_SEQUENCE("simpleSeq: seq=%d: w4 triggered\n", seq_id);
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext->getNode();
  num_nodes = theContext->getNumNodes();

  //#define SIMPLE_SEQ_MULTI_NODE 1
  #define SIMPLE_SEQ_PARALLEL 1

  #if SIMPLE_SEQ_MULTI_NODE
  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }
  #endif

  #if SIMPLE_SEQ
  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, simpleSeq);

    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 20);

    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 100);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 200);

    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 1000);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 2000);

    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10000);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 20000);
  }
  #endif

  #if SIMPLE_SEQ_MULTI_NODE
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
  #endif

  #if SIMPLE_SEQ_SINGLE_NODE
  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, mySeqSingleNode);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 30);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10);
  }
  #endif

  #if SIMPLE_SEQ_PARALLEL
  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, mySeqParallel);
    theMsg->sendMsg<EmptyMsg, action1>(0, makeSharedMessage<EmptyMsg>(), 10);
  }
  #endif

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
