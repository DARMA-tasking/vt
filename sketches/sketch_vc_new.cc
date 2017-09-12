
// runtime
struct MyMainContext : vt::VirtualContextMessage {
  int argc;
  char** argv;
};

/// user

struct MyMainContext : VirtualContextMain {
  virtual_context_collection_id_t my_particle_collection;
};

void main_context_handler(SystemMesasge* msg, MyMainContext* main_context) {
  main_context->is_migrateable = true;

  main_context->my_particle_collection =
    the_virtual->create_virtual_context_collection<>(...);
}

REGISTER_MAIN_CONTEXT(MyMainContext);

