
// sketched out example interface to VirtualContextCollection, just for
// pedagogical purposes
template <typename Idx>
struct VirtualContextCollection {
  Idx index();
  Idx min_index();
  Idx max_index();

  VirtualContextID get_id(Idx i);
};

// define some messages, these should go away with the DARMA front-end which
// will provide parameterization
struct HanMsg : vt::Message {
  RDMA_HandleType han;
  HanMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

struct WorkMsg : vt::Message {
  WorkMsg() : Message() { }
};

struct SizeMsg : vt::Message {
  int size;
  SizeMsg(int const& in_size) : Message(), size(in_size) { }
};

struct FinishedMsg : vt::Message {
  bool fin;
  FinishedMsg(bool in_fin) : Message(), fin(in_fin) { }
};

// the virtual context for a Jacobi1D block
struct Jacobi1D : VirtualContextCollection<Index1D<int>> {
  double* v1 = nullptr, *v2 = nullptr;
  double lb = 0.0, rb = 0.0;
  int size = 0;
  int cur_iter = 0;
  bool conv = false;

  Index1D<int> left_neigh, right_neigh;

  RDMA_HandleType my_lb_h = no_rdma_handle, my_rb_h = no_rdma_handle;
  RDMA_HandleType n_lb_h = no_rdma_handle, n_rb_h = no_rdma_handle;

  Jacobi1D(int const& in_size) : size(in_size) {
    v1 = new double[size];
    v2 = new double[size];

    left_neigh = index()-1 < 0 ? max_index()-1 : index()-1;
    right_neigh = index()+1 > max_index()-1 ? 0 : index()+1;
  }

  // set up RDMA for boundary exchange... this is just an example. this should
  // be done somewhat "automatically" for the user eventually in the
  // frontend. also in this case since the boundary is literally one element it
  // doesn't actually make sense to use RDMA, but with a 2/3D Jacobi, it would
  // improve performance
  void setup_rdma() {
    my_lb_h = register_new_typed_rdma_handler(&lb, 1);
    my_rb_h = register_new_typed_rdma_handler(&rb, 1);

    parallel([]{
      sendMsg<HanMsg, recv_han_left>(
        get_id(left_neigh), make_shared_message<HanMsg>(my_rb_h)
      );
      sendMsg<HanMsg, recv_han_right>(
        get_id(right_neigh), make_shared_message<HanMsg>(my_lb_h)
      );
      wait<HanMsg, recv_han_left>([](HanMsg* m){
        n_lb_h = m->han;
      });
      wait<HanMsg, recv_han_right>([](HanMsg* m){
        n_rb_h = m->han;
      });
    });
  }

  void do_kernel() {
    /* ... do kernel work ... */
    /* set local converged based on local error */
    conv = local_converged_value;
  }

  bool not_converged() const { return !conv; }
};

void reduce_handler_fn(ReduceMsg* m) {
  broadcast_msg(m->j1d, make_shared_message<FinishedMsg>(m->result));
}

void do_jacobi_work(WorkMsg* msg, Jacobi1D* j) {
  j->setup_rdma();
  while_sequence(j->conv, []{
    parallel([]{
      rdma_put<put_left>(j->n_lb_h, &j->v1[0], 1);
      rdma_put<put_right>(j->n_rb_h, &j->v1[j->size-1], 1);
      wait<put_left>();
      wait<put_right>();
    });
    j->do_kernel();
    j->cur_iter++;
  });
}

constexpr int const num_async_iter = 10;

void do_jacobi_work_async(WorkMsg* msg, Jacobi1D* j) {
  j->setup_rdma();
  while_sequence(j->conv, [=]{
    parallel([]{
      rdma_put<put_left>(j->n_lb_h, &j->v1[0], 1);
      rdma_put<put_right>(j->n_rb_h, &j->v1[j->size-1], 1);
      wait<put_left>();
      wait<put_right>();
    });
    j->do_kernel();
    if (j->cur_iter % num_async_iter == 0) {
      j->reduce<reduce_handler_fn>(j->conv, LOGICAL_ADD_OP);
    }
    if ((j->cur_iter-1) % num_async_iter == 0) {
      wait<FinishedMsg, reduce_result>([](FinishedMsg* result) {
        j->conv = result->fin;
      });
    }
    j->cur_iter++;
  });
}


Jacobi1D* create_han(SizeMsg* msg, Index1D<int> idx) {
  return new Jacobi1D(idx, msg->size);
}

void darma_main_task(std::vector<std::string>> args) {
  int const num_elm = 64;
  SizeMsg* m = make_shared_message<SizeMsg>(128);
  VirtualContextID jcid = create_vc_collection<SizeMsg, Jacobi1D, create_han>(
    num_elm, m
  );
  sendMsg<WorkMsg, do_jacobi_work>(
    jcid, make_shared_message<WorkMsg>()
  );
}

