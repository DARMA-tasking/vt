
#include "transport.h"
#include <cstdlib>

using namespace vt;

struct ParticleCollection : vt::VirtualCollectionContext {
  std::vector<Particle> particles;

  NodeType initial_map_to_node(index i) {
    
  }
};

struct MainContext : vt::VirtualContext {
  int some_info;
};

void initialize_particles(
  vt::BaseMessage* in_msg, IndexedVirtualContext* virtual_ctx
) {
  // adds ome parties to virtual_ctx
}

void main_function() {
  auto collection_proxy = the_virtual->register_new_collection<ParticleCollection>(10000);
  the_virtual->broadcast_msg(collection_proxy, initialize_particles_handles);

  auto collection_proxy_elem_t = the_virtual->get_index(collection_proxy, index);
  the_virtual->migrate_to_node(collection_proxy_elem_t, 5);
}


// int main(int argc, char** argv) {
//   CollectiveOps::initialize_context(argc, argv);
//   CollectiveOps::initialize_runtime();

//   hello_world_han = CollectiveOps::register_handler(hello_world);

//   auto const& my_node = the_context->get_node();
//   auto const& num_nodes = the_context->get_num_nodes();

//   if (num_nodes == 1) {
//     fprintf(stderr, "Please run with at least two ranks!\n");
//     fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
//     exit(1);
//   }

//   if (my_node == 0) {
//     HelloMsg* msg = new HelloMsg(my_node);
//     the_msg->broadcast_msg(hello_world_han, msg, [=]{ delete msg; });

//     // Example of  to use a system managed message with transfer of control
//     // HelloMsg* msg = make_shared_message<HelloMsg>(my_node);
//     // the_msg->broadcast_msg(hello_world_han, msg);
//   }

//   while (1) {
//     the_msg->scheduler();
//   }

//   return 0;
// }

struct VirtualContextCollection {

};

void my_register_virtual_context_collection() {
  vc_proxy_collection_t hello_proxy = the_virtual->make_vc_collection_collective(1000);

  the_virtual->send_msg(hello_proxy, virtual_han, msg);
}

struct VirtualContextManager {
  void
  send_msg(..);

private:
  std::unordered_map<vc_proxy_t, VirtualContext*> local_contexts;
};

struct VirtualContext {
  vc_proxy_t context_proxy;
};

struct MyHelloContext : vt::VirtualContext {
  int some_info;
};

static void hello_world_virtual(vt::BaseMessage* in_msg, VirtualContext* virtual_ctx) {
  HelloMsg& msg = *static_cast<HelloMsg*>(in_msg);
  MyHelloContext& hello_context = *static_cast<MyHelloContext*>(virtual_ctx);

  hello_context->some_info++;
}


void my_register_virtual_context() {
  MyHelloContext* hello_context = new MyHelloContext();
  vc_proxy_t hello_proxy = the_virtual->register_virtual_context(hello_context);
  vc_proxy_HandlerType virtual_han = the_virtual->register_virtual_handler(hello_world_virtual);
  HelloMsg* msg = new HelloMsg(my_node);

  the_virtual->send_msg(hello_proxy, virtual_han, msg);


  // MyHelloContext* hello_context = new MyHelloContext();
  // vc_proxy_t hello_proxy = the_virtual->make_virtual_context<MyHelloContext>(/*construct args*/);

  // MyHelloContext* hello_context = new MyHelloContext();
  // vc_proxy_t hello_proxy = the_virtual->make_virtual_context<MyHelloContext>(8);

  //the_msg->send_msg(10, handler, msg);

  // vc_proxy_t: 64-bits: (32 id) (16 node) (few other control bits)
}
