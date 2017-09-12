
struct VirtualContext {

};

struct MigrateableVirtualContext : VirtualContext {

};

struct NonMigrateableVirtualContext : VirtualContext {

};

struct SequencedContext {
};

struct MyContext : SequencedContext {

  void handler_msg() {

  };

  void handler2() {

  }

  void sequence() {
    for_sequence_builder(0, num_iters, []{
      auto cur_iter = get_current_iter();
      when_handler(HandlerTypeype_1, cur_iter, []{

      });
      when_handler(HandlerTypeype_2, tag, []{

      }).next_hanlder(HandlerTypeype_3, tag, []{

      });
    });
  }
};

void seq() {
  sequence.create(fst_msg_han, [=]{
  }).next(snd_msg_han, [=]{
  });

}








struct VirutalContext {

};

template <typename Idx>
struct IndexedState : VirtualContextCollection {

};

struct JacobiData : IndexedState<Index<int>> {
  double *t1 = nullptr, *t2 = nullptr;
  size_t block_size = 0, num_elems = 0;

  data_view<double*> left, right, top, bottom;

  JacobiData(
    size_t const& block, num_elems const& elems
  ) : block_size(block), num_elems(elems);

  NodeType node_map(Index<int> i) {
    return i.get() % the_context->get_num_nodes();
  }

  void initialize(size_t const& block_size) {
    v1 = new double[block_size];
    v2 = new double[block_size];
  }

  void migrate(JacobiData* jd) {

  }

  static JacobiData*
  construct(Index<int> i, size_t const& num_elems) {
    JacobiData* jd = new JacobiData();
    return jd;
  }
};


void define_work_sequence(JacobiData<Index<int>>* jd) {
  sequenced(init_handler, []{
    jd = JacobiData::construct(num_elems, block_size);
    register_rdma_data(left, t1, ..);
    register_rdma_data(right, t1, ..);
  }).next(
    for_loop(0, num_iters,
      sequenced([=]{
        put_data(jd.index+1, cur_iter, left_han);
        put_data(jd.index-1, cur_iter, right_han);

        when_put_handler(left_han, cur_iter, []{
          memcpy(jd->v1, data_ptr, data_size);
        });
        when_put_handler(right_han, cur_iter, []{
          memcpy(jd->v1, data_ptr, data_size);
        });

      }).next([=]{
        jd->run_kernel();
      });
    );
  });
}
