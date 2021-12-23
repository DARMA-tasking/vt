/*
//@HEADER
// *****************************************************************************
//
//                                  untyped.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_INDEX_UNTYPED_H
#define INCLUDED_VT_VRT_COLLECTION_INDEX_UNTYPED_H

#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace index {

using RegisteredIndexType = uint16_t;

template <unsigned num_bytes>
struct UntypedIndex {
  UntypedIndex() = default;

  template <typename IdxT>
  explicit UntypedIndex(IdxT const& in_idx);

  bool operator==(UntypedIndex<num_bytes> const& other) const;

  bool operator!=(UntypedIndex<num_bytes> const& other) const {
    return !(*this == other);
  }

  RegisteredIndexType idx_ = 0;
  std::array<char, num_bytes> idx_bytes_;
};

namespace registry {

template <unsigned num_bytes>
struct IndexInfo {
  using DispatchFnType = std::function<void(UntypedIndex<num_bytes>* msg)>;
  using IsEqualFnType = std::function<
    bool(UntypedIndex<num_bytes> const& a, UntypedIndex<num_bytes> const& b)
  >;

  IndexInfo(
    RegisteredIndexType in_idx,
    std::size_t in_bytes,
    bool in_is_bytecopyable,
    DispatchFnType in_dispatch,
    IsEqualFnType in_is_equal
  ) : idx_(in_idx),
      bytes_(in_bytes),
      is_bytecopyable_(in_is_bytecopyable),
      dispatch_(in_dispatch),
      is_equal_(in_is_equal)
  { }

  RegisteredIndexType idx_ = -1;
  std::size_t bytes_ = 0;
  bool is_bytecopyable_ = false;
  DispatchFnType dispatch_;
  IsEqualFnType is_equal_;
};

 static constexpr unsigned const base_num_bytes = 48;

using RegistryType = std::vector<IndexInfo<base_num_bytes>>;

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

template <unsigned num_bytes>
inline auto isEqual(
  RegisteredIndexType han,
  UntypedIndex<num_bytes> const& a, UntypedIndex<num_bytes> const& b
) {
  return getRegistry().at(han).is_equal_(a, b);
}

template <typename IdxT>
inline RegisteredIndexType makeIdx() {
  return Type<IdxT>::idx;
}

template <typename IdxT>
inline std::array<char, base_num_bytes> makeBytes(IdxT const& in_idx) {
  vtAssert(sizeof(IdxT) < base_num_bytes, "Must fit");
  std::array<char, base_num_bytes> out;
  *reinterpret_cast<IdxT*>(out.data()) = in_idx;
  return out;
}

template <typename IdxT>
Registrar<IdxT>::Registrar() {
  static constexpr bool const is_bytecopyable = checkpoint::SerializableTraits<IdxT>::is_bytecopyable;

  using Untyped = UntypedIndex<base_num_bytes>;

  auto& reg = getRegistry();
  index = reg.size();
  reg.emplace_back(
    IndexInfo<base_num_bytes>{
      index,
      sizeof(IdxT),
      is_bytecopyable,
      [](Untyped* msg){
        //@todo: handle serialization case with enable_if on Registrar
        char const* const data = msg->idx_bytes_.data();
        IdxT const reconstructed_idx = *reinterpret_cast<IdxT const*>(data);
        fmt::print("the index is {}\n", reconstructed_idx);
      },
      [](Untyped const& a, Untyped const& b) -> bool {
        return *reinterpret_cast<IdxT const*>(a.idx_bytes_.data()) ==
               *reinterpret_cast<IdxT const*>(b.idx_bytes_.data());
      }
    }
  );
}

} /* end namespace registry */

template <unsigned num_bytes>
template <typename IdxT>
UntypedIndex<num_bytes>::UntypedIndex(IdxT const& in_idx)
  : idx_(registry::makeIdx<IdxT>()),
    idx_bytes_(registry::makeBytes(in_idx))
{ }

template <unsigned num_bytes>
bool UntypedIndex<num_bytes>::operator==(
  UntypedIndex<num_bytes> const& other
) const {
  return idx_ == other.idx_ and registry::isEqual(idx_, *this, other);
}

}}}} /* end namespace vt::vrt::collection::index */

#endif /*INCLUDED_VT_VRT_COLLECTION_INDEX_UNTYPED_H*/
