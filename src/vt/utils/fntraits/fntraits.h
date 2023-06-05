/*
//@HEADER
// *****************************************************************************
//
//                                  fntraits.h
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

#if !defined INCLUDED_VT_UTILS_FNTRAITS_FNTRAITS_H
#define INCLUDED_VT_UTILS_FNTRAITS_FNTRAITS_H

namespace vt::util::fntraits::detail {

struct NoMsg {};

template <typename enabled, typename... Args>
struct ObjFuncTraitsImpl;

template <typename Obj, typename Return, typename Msg>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    (std::is_convertible<Msg*, vt::Message*>::value or
    std::is_convertible<Msg*, vt::ShortMessage*>::value or
    std::is_convertible<Msg*, vt::EpochMessage*>::value or
    std::is_convertible<Msg*, vt::PayloadMessage*>::value)
    and
    std::is_pointer<Obj>::value
  >,
  Return(*)(Obj, Msg*)
> {
  static constexpr bool is_member = false;
  using ObjT = std::remove_pointer_t<Obj>;
  using MsgT = Msg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<MsgT>;
};

template <typename Obj, typename Return>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    not (
      std::is_convertible<Obj, vt::Message*>::value or
      std::is_convertible<Obj, vt::ShortMessage*>::value or
      std::is_convertible<Obj, vt::EpochMessage*>::value or
      std::is_convertible<Obj, vt::PayloadMessage*>::value
    ) and
    std::is_pointer<Obj>::value
  >,
  Return(*)(Obj)
> {
  static constexpr bool is_member = false;
  using ObjT = std::remove_pointer_t<Obj>;
  using MsgT = NoMsg;
  using Arg1 = Obj;
  using ReturnT = Return;
  using TupleType = std::tuple<>;
  template <template <typename...> class U>
  using WrapType = U<>;
};

template <typename Obj, typename Return, typename Arg, typename... Args>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    not (
      std::is_convertible<Arg, vt::Message*>::value or
      std::is_convertible<Arg, vt::ShortMessage*>::value or
      std::is_convertible<Arg, vt::EpochMessage*>::value or
      std::is_convertible<Arg, vt::PayloadMessage*>::value
    )
  >,
  Return(*)(Obj*, Arg, Args...)
> {
  static constexpr bool is_member = false;
  using ObjT = Obj;
  using MsgT = NoMsg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<std::decay_t<Arg>, std::decay_t<Args>...>;
  using TupleType = WrapType<std::tuple>;
};

template <typename Obj, typename Return, typename Msg>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    std::is_convertible<Msg*, vt::Message*>::value or
    std::is_convertible<Msg*, vt::ShortMessage*>::value or
    std::is_convertible<Msg*, vt::EpochMessage*>::value or
    std::is_convertible<Msg*, vt::PayloadMessage*>::value
  >,
  Return(Obj::*)(Msg*)
> {
  static constexpr bool is_member = true;
  using ObjT = Obj;
  using MsgT = Msg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<MsgT>;
};

template <typename Obj, typename Return>
struct ObjFuncTraitsImpl<
  std::enable_if_t<std::is_same_v<void, void>>,
  Return(Obj::*)()
> {
  static constexpr bool is_member = true;
  using ObjT = Obj;
  using MsgT = NoMsg;
  using TupleType = std::tuple<>;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<>;
};

template <typename Obj, typename Return, typename Arg, typename... Args>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    not (
      std::is_convertible<Arg, vt::Message*>::value or
      std::is_convertible<Arg, vt::ShortMessage*>::value or
      std::is_convertible<Arg, vt::EpochMessage*>::value or
      std::is_convertible<Arg, vt::PayloadMessage*>::value
    )
  >,
  Return(Obj::*)(Arg, Args...)
> {
  static constexpr bool is_member = true;
  using ObjT = Obj;
  using MsgT = NoMsg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<std::decay_t<Arg>, std::decay_t<Args>...>;
  using TupleType = WrapType<std::tuple>;
};

template <typename Return, typename Msg>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    std::is_convertible<Msg*, vt::Message*>::value or
    std::is_convertible<Msg*, vt::ShortMessage*>::value or
    std::is_convertible<Msg*, vt::EpochMessage*>::value or
    std::is_convertible<Msg*, vt::PayloadMessage*>::value
  >,
  Return(*)(Msg*)
> {
  static constexpr bool is_member = false;
  using MsgT = Msg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<MsgT>;
};

template <typename Return>
struct ObjFuncTraitsImpl<
  std::enable_if_t<std::is_same_v<void, void>>,
  Return(*)()
> {
  static constexpr bool is_member = false;
  using MsgT = NoMsg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<>;
  using TupleType = std::tuple<>;
};

template <typename Return, typename Arg, typename... Args>
struct ObjFuncTraitsImpl<
  std::enable_if_t<
    not std::is_pointer<Arg>::value
  >,
  Return(*)(Arg, Args...)
> {
  static constexpr bool is_member = false;
  using MsgT = NoMsg;
  using Arg1 = Arg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<std::decay_t<Arg>, std::decay_t<Args>...>;
  using TupleType = WrapType<std::tuple>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename enabled, typename... Args>
struct FunctorTraitsImpl;

template <typename FunctorT, typename Return, typename Msg>
struct FunctorTraitsImpl<
  std::enable_if_t<
    std::is_convertible<Msg*, vt::Message*>::value or
    std::is_convertible<Msg*, vt::ShortMessage*>::value or
    std::is_convertible<Msg*, vt::EpochMessage*>::value or
    std::is_convertible<Msg*, vt::PayloadMessage*>::value
  >,
  FunctorT,
  Return(FunctorT::*)(Msg*)
> {
  static constexpr bool is_member = false;
  using MsgT = Msg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<MsgT>;
  using FuncPtrType = Return(*)(Msg*);
};

template <typename FunctorT, typename Return>
struct FunctorTraitsImpl<
  std::enable_if_t<std::is_same_v<void, void>>,
  FunctorT,
  Return(FunctorT::*)()
> {
  static constexpr bool is_member = false;
  using MsgT = NoMsg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<>;
  using TupleType = std::tuple<>;
  using FuncPtrType = Return(*)();
};

template <typename FunctorT, typename Return, typename Arg, typename... Args>
struct FunctorTraitsImpl<
  std::enable_if_t<
    not (
      std::is_convertible<Arg, vt::Message*>::value or
      std::is_convertible<Arg, vt::ShortMessage*>::value or
      std::is_convertible<Arg, vt::EpochMessage*>::value or
      std::is_convertible<Arg, vt::PayloadMessage*>::value
    )
  >,
  FunctorT,
  Return(FunctorT::*)(Arg, Args...)
> {
  static constexpr bool is_member = false;
  using MsgT = NoMsg;
  using Arg1 = Arg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<std::decay_t<Arg>, std::decay_t<Args>...>;
  using TupleType = WrapType<std::tuple>;
  using FuncPtrType = Return(*)(Arg, Args...);
};

template <typename FunctorT, typename Return, typename Msg>
struct FunctorTraitsImpl<
  std::enable_if_t<
    std::is_convertible<Msg*, vt::Message*>::value or
    std::is_convertible<Msg*, vt::ShortMessage*>::value or
    std::is_convertible<Msg*, vt::EpochMessage*>::value or
    std::is_convertible<Msg*, vt::PayloadMessage*>::value
  >,
  FunctorT,
  Return(FunctorT::*)(Msg*) const
> {
  static constexpr bool is_member = false;
  using MsgT = Msg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<MsgT>;
  using FuncPtrType = Return(*)(Msg*);
};

template <typename FunctorT, typename Return>
struct FunctorTraitsImpl<
  std::enable_if_t<std::is_same_v<void, void>>,
  FunctorT,
  Return(FunctorT::*)() const
> {
  static constexpr bool is_member = false;
  using MsgT = NoMsg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<>;
  using TupleType = std::tuple<>;
  using FuncPtrType = Return(*)();
};

template <typename FunctorT, typename Return, typename Arg, typename... Args>
struct FunctorTraitsImpl<
  std::enable_if_t<
    not (
      std::is_convertible<Arg, vt::Message*>::value or
      std::is_convertible<Arg, vt::ShortMessage*>::value or
      std::is_convertible<Arg, vt::EpochMessage*>::value or
      std::is_convertible<Arg, vt::PayloadMessage*>::value
    )
  >,
  FunctorT,
  Return(FunctorT::*)(Arg, Args...) const
> {
  static constexpr bool is_member = false;
  using MsgT = NoMsg;
  using Arg1 = Arg;
  using ReturnT = Return;
  template <template <typename...> class U>
  using WrapType = U<std::decay_t<Arg>, std::decay_t<Args>...>;
  using TupleType = WrapType<std::tuple>;
  using FuncPtrType = Return(*)(Arg, Args...);
};

///////////////////////////////////////////////////////////////////////////////

template <typename enabled, typename... Args>
struct CBTraitsImpl;

template <typename Msg>
struct CBTraitsImpl<
  std::enable_if_t<
    std::is_convertible<Msg*, vt::Message*>::value or
    std::is_convertible<Msg*, vt::ShortMessage*>::value or
    std::is_convertible<Msg*, vt::EpochMessage*>::value or
    std::is_convertible<Msg*, vt::PayloadMessage*>::value
  >,
  Msg
> {
  using MsgT = std::remove_pointer_t<Msg>;
};

template <>
struct CBTraitsImpl<
  std::enable_if_t<std::is_same_v<void, void>>
> {
  using MsgT = NoMsg;
  using TupleType = std::tuple<>;
};

template <typename Arg, typename... Args>
struct CBTraitsImpl<
  std::enable_if_t<
    not (
      std::is_convertible<Arg*, vt::Message*>::value or
      std::is_convertible<Arg*, vt::ShortMessage*>::value or
      std::is_convertible<Arg*, vt::EpochMessage*>::value or
      std::is_convertible<Arg*, vt::PayloadMessage*>::value
    )
  >,
  Arg,
  Args...
> {
  using MsgT = NoMsg;
  using TupleType = std::tuple<std::decay_t<Arg>, std::decay_t<Args>...>;
};


} /* end namespace vt::util::fntraits::detail */

///////////////////////////////////////////////////////////////////////////////

namespace vt {

template <typename... Args>
struct ObjFuncTraits : util::fntraits::detail::ObjFuncTraitsImpl<void, Args...> {};

template <typename... Args>
struct FuncTraits : util::fntraits::detail::ObjFuncTraitsImpl<void, Args...> {};

template <typename... Args>
struct FunctorTraits : util::fntraits::detail::FunctorTraitsImpl<void, Args...> {};

template <typename... Args>
struct CBTraits : util::fntraits::detail::CBTraitsImpl<void, Args...> {};

using NoMsg = util::fntraits::detail::NoMsg;

} /* end namespace vt */

#endif /*INCLUDED_VT_UTILS_FNTRAITS_FNTRAITS_H*/
