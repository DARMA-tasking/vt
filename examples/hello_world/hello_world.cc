/*
//@HEADER
// *****************************************************************************
//
//                                hello_world.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include <vt/transport.h>

using RegisteredIndexType = int;

struct IndexMessage : vt::Message {
  vt::VirtualProxyType proxy = vt::no_vrt_proxy;
  RegisteredIndexType idx = -1;
  std::array<char, 48> bytes;
};

struct IndexInfo {
  using DispatchFnType = std::function<void(IndexMessage* msg)>;

  IndexInfo(
    RegisteredIndexType in_idx,
    std::size_t in_bytes,
    bool in_is_bytecopyable,
    DispatchFnType in_dispatch
  ) : idx_(in_idx),
      bytes_(in_bytes),
      is_bytecopyable_(in_is_bytecopyable),
      dispatch_(in_dispatch)
  { }

  RegisteredIndexType idx_ = -1;
  std::size_t bytes_ = 0;
  bool is_bytecopyable_ = false;
  DispatchFnType dispatch_;
};

using RegistryType = std::vector<IndexInfo>;

inline RegistryType& getRegistry() {
  static RegistryType reg;
  return reg;
}

template <typename IdxT>
struct Registrar {
  Registrar();
  RegisteredIndexType index;
};

template <typename IdxT>
struct Type {
  static RegisteredIndexType const idx;
};

template <typename IdxT>
RegisteredIndexType const Type<IdxT>::idx = Registrar<IdxT>().index;

inline auto getDispatch(RegisteredIndexType han) {
  return getRegistry().at(han).dispatch_;
}

template <typename IdxT>
inline RegisteredIndexType makeIdx() {
  return Type<IdxT>::idx;
}

template <typename IdxT>
Registrar<IdxT>::Registrar() {
  static constexpr bool const is_bytecopyable = checkpoint::SerializableTraits<IdxT>::is_bytecopyable;

  auto& reg = getRegistry();
  index = reg.size();
  reg.emplace_back(
    IndexInfo{
      index,
      sizeof(IdxT),
      is_bytecopyable,
      [](IndexMessage* msg){
        //@todo: handle serialization case with enable_if on Registrar
        char const* const data = msg->bytes.data();
        IdxT const reconstructed_idx = *reinterpret_cast<IdxT const*>(data);
        fmt::print("the index is {}\n", reconstructed_idx);

        /**
           // collection manager code
           auto proxy = msg->proxy;
           auto elm_holder = theCollection()->findElmHolder<IdxT>(proxy);
           auto& inner_holder = elm_holder->lookup(idx);
           auto const col_ptr = inner_holder.getRawPtr();
           // deliver message to col_ptr handler
         */
      }
    }
  );
}

template <typename IndexT>
std::array<char, 48> toBytes(IndexT idx) {
  //@todo: handle serialization case
  vtAssert(sizeof(IndexT) < 48, "Must fit");
  std::array<char, 48> out;
  char* data = out.data();
  *reinterpret_cast<IndexT*>(data) = idx;
  return out;
}

void testHandler(IndexMessage* msg) {
  getDispatch(msg->idx)(msg);
}

template <typename IndexT>
void sendIndex(IndexT idx) {
  auto x = makeIdx<IndexT>();
  auto m = vt::makeMessage<IndexMessage>();
  m->idx = x;
  m->bytes = toBytes(idx);
  vt::theMsg()->sendMsg<IndexMessage, testHandler>(1, m);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  if (this_node == 0) {
    vt::Index3D idx{10, -20, 55};
    sendIndex(idx);
  }

  vt::finalize();

  return 0;
}
