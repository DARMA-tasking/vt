using namespace vt;
using namespace vt::vrt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct EmptyMsg : vt::Message {
  EmptyMsg() : Message() { }
};

struct MyVC : vt::vrt::VrtContext {
  int my_data = 10;

  MyVC(int const& my_data_in) : my_data(my_data_in) { }
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

void my_fun(EmptyMsg* msg) {
  PRINT_SEQUENCE("action1 WAIT-1 triggered\n");
}

struct ParticleBox : vt::vrt::VrtContext {
  std::vector<Particle> box;

  void do_kernel(int x);
  void process_left_msg();

  
};

 /*ParticleBox* box*/

static void my_fun(ParticleBox* acc) {

}

static void outer_my_seq(SeqType const& my_id) {
  theSeq->for_loop_migrateable<my_seq>(0, 100);
}

static void my_seq(SeqType const& seq_id /*vt::accessor<ParticleBox> acc*/) {
  //theSeq->for_loop(0, niter, 1, [=](ForIndex i){
    theSeq->forall_loop(0, neighbors, 1, [=](ForIndex j){
      theSeq->wait<EmptyMsg, action1>([=]{
        if (neighbor == LEFT) {
          box->process_left_msg();
        } else if (neighbor == RIGHT) {
          box->process_right_msg();
        }
      });
    });

    //theSeq->sequenced(box, &ParticleBox::do_kernel, 10);
    //theSeq->wait<EmptyMsg, action1>([](EmptyMsg* msg){});

    theSeq->wait<ParticleBox, EmptyMsg, action1>([](EmptyMsg* msg, ParticleBox* box){
    });

    theSeq->migratable_sequence(my_fun);

    theSeq->sequenced([](/*ParticleBox* box*/){
      auto box = theSeq->getVC();
      //auto box = static_cast<ParticleBox*>(theSeq->getCurrentVirtualContext());
      //auto box = acc->get();
      box->do_kernel();
    });

    theSeq->sequenced([=]{
      box->contirbute_to_reduction(my_local_converged_value);
    });
    //});
}

static void mySeq(SeqType const& seq_id) {
  PRINT_SEQUENCE("mySeq: executing sequence\n");

  theSeq->wait<EmptyMsg, action1, my_fun>(10, my_fun);

  theSeq->sequenced([]{
    PRINT_SEQUENCE("action1 sequenced_block triggered\n");
    auto const& my_node = theContext->getNode();
    theMsg->sendMsg<EmptyMsg, action1>(
      my_node, makeSharedMessage<EmptyMsg>(), 20
    );
  });

}

