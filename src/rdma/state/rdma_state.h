
#if !defined INCLUDED_RDMA_RDMA_STATE_H
#define INCLUDED_RDMA_RDMA_STATE_H

#include "config.h"
#include "rdma/rdma_common.h"
#include "rdma/rdma_handle.h"
#include "rdma/rdma_msg.h"
#include "rdma/rdma_info.h"

#include "rdma/group/rdma_map.h"
#include "rdma/group/rdma_group.h"

#include "rdma/channel/rdma_channel.h"

#include <unordered_map>
#include <vector>

namespace vt { namespace rdma {

static constexpr RDMA_HandlerType const first_rdma_handler = 1;

struct State {
  using RDMA_InfoType = Info;
  using RDMA_TypeType = Type;
  using RDMA_MapType = Map;
  using RDMA_GroupType = Group;
  using RDMA_GetFunctionType = ActiveGetFunctionType;
  using RDMA_PutFunctionType = ActivePutFunctionType;
  template <typename MsgType>
  using RDMA_GetTypedFunctionType = ActiveTypedGetFunctionType<MsgType>;
  template <typename MsgType>
  using RDMA_PutTypedFunctionType = ActiveTypedPutFunctionType<MsgType>;
  using RDMA_TagGetHolderType =
    std::tuple<RDMA_GetFunctionType, RDMA_HandlerType>;
  using RDMA_TagPutHolderType =
    std::tuple<RDMA_PutFunctionType, RDMA_HandlerType>;
  using RDMA_FunctionType = BaseMessage;

  template <typename T>
  using TagContainerType = std::unordered_map<TagType, T>;

  template <typename T>
  using ContainerType = std::vector<T>;

  RDMA_HandleType handle = no_rdma_handle;
  RDMA_PtrType ptr = no_rdma_ptr;
  ByteType num_bytes = no_byte;

  State(
    RDMA_HandleType const& in_handle, RDMA_PtrType const& in_ptr = no_rdma_ptr,
    ByteType const& in_num_bytes = no_byte,
    bool const& use_default_handler = false
  );

  template <typename AssocFuncT, typename FuncT>
  RDMA_HandlerType setRDMAGetFn(
    AssocFuncT* msg, FuncT const& fn, bool const& any_tag = false,
    TagType const& tag = no_tag
  );

  template <typename AssocFuncT, typename FuncT>
  RDMA_HandlerType setRDMAPutFn(
    AssocFuncT* msg, FuncT const& fn, bool const& any_tag = false,
    TagType const& tag = no_tag
  );

  void unregisterRdmaHandler(
    RDMA_TypeType const& type, TagType const& tag, bool const& use_default
  );

  void unregisterRdmaHandler(
    RDMA_HandlerType const& handler, TagType const& tag
  );

  RDMA_HandlerType makeRdmaHandler(RDMA_TypeType const& rdma_type);

  bool testReadyGetData(TagType const& tag);
  bool testReadyPutData(TagType const& tag);

  void getData(
    GetMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info
  );

  void putData(
    PutMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info
  );

  void processPendingGet(TagType const& tag = no_tag);
  void setDefaultHandler();

  static RDMA_GetType defaultGetHandlerFn(
    StateMessage<State>* msg, ByteType num_bytes, ByteType req_offset,
    TagType tag, bool is_local
  );

  static void defaultPutHandlerFn(
    StateMessage<State>* msg, RDMA_PtrType in_ptr, ByteType in_num_bytes,
    ByteType req_offset, TagType tag, bool is_local
  );

  bool using_default_put_handler = false;
  bool using_default_get_handler = false;

  std::unique_ptr<RDMA_GroupType> group_info = nullptr;

private:
  RDMA_HandlerType this_rdma_get_handler = uninitialized_rdma_handler;
  RDMA_HandlerType this_rdma_put_handler = uninitialized_rdma_handler;

  bool get_any_tag = false;
  bool put_any_tag = false;

  RDMA_GetFunctionType rdma_get_fn = no_action;
  RDMA_PutFunctionType rdma_put_fn = no_action;

  TagContainerType<RDMA_TagGetHolderType> get_tag_holder;
  TagContainerType<RDMA_TagPutHolderType> put_tag_holder;
  TagContainerType<ContainerType<RDMA_InfoType>> pending_tag_gets;
  TagContainerType<ContainerType<RDMA_InfoType>> pending_tag_puts;

  RDMA_FunctionType* user_state_get_msg_ = nullptr;
  RDMA_FunctionType* user_state_put_msg_ = nullptr;
};

}} //end namespace vt::rdma

#include "rdma/state/rdma_state.impl.h"

#endif /*INCLUDED_RDMA_RDMA_STATE_H*/
