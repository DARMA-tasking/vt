
// runtime code
struct VirtualContext {
  using virtual_context_id_t = int64_t;
  bool is_migrateable = false;

  virtual_context_id_t get_virtual_context_id();
};

struct VirtualContextMessage : vt::Message {

};

// user code
struct MyContext : VirtualContext {
  int my_data;

  MyContext() = default;
};

struct MyMsg : vt::VirtualContextMessage {
  int size, info;
};

MyContext* my_handler(MyMsg* msg) {
  return new MyContext(size, info);
}

// work msg for MyContext
struct MyWorkMsg : vt::VirtualContextMessage {
  int work_info;
};

void my_work_handler(MyWorkMsg* msg, MyContext* context) {
  // do work on context
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 5) {
    MyMsg* msg = make_shared_message<MyMsg>(10, 100);

    virtual_context_id_t my_id = the_virtual->create_virtual_context<
      MyContext, MyMsg, my_handler
    >(msg);

    MyWorkMsg* msg = make_shared_message<MyWorkMsg>();
    the_virtual->sendMsg<MyContext, MyWorkMsg, my_work_handler>(my_id, work_msg);
  }

  while (1) {
    theMsg->scheduler();
  }

  return 0;
}
