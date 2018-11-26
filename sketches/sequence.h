/*
//@HEADER
// ************************************************************************
//
//                          sequence.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

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
    return i.get() % theContext()->get_num_nodes();
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
