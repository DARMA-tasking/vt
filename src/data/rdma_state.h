
#if ! defined __RUNTIME_TRANSPORT_RDMASTATE__
#define __RUNTIME_TRANSPORT_RDMASTATE__

#include "common.h"
#include "rdma_common.h"
#include "rdma_map.h"
#include "rdma_handle.h"
#include "rdma_msg.h"
#include "rdma_channel.h"
#include "rdma_info.h"
#include "rdma_group.h"

#include <unordered_map>
#include <vector>

namespace runtime { namespace rdma {

static constexpr RDMA_HandlerType const first_rdma_handler = 1;

struct State {
  using RDMA_InfoType = Info;
  using RDMA_TypeType = Type;
  using RDMA_MapType = Map;
  using RDMA_GroupType = Group;
  using RDMA_GetFunctionType = ActiveGetFunctionType;
  using RDMA_PutFunctionType = ActivePutFunctionType;
  using RDMA_TagGetHolderType = std::tuple<RDMA_GetFunctionType, RDMA_HandlerType>;
  using RDMA_TagPutHolderType = std::tuple<RDMA_PutFunctionType, RDMA_HandlerType>;

  template <typename T>
  using tag_container_t = std::unordered_map<TagType, T>;

  template <typename T>
  using container_t = std::vector<T>;

  RDMA_HandleType handle = no_rdma_handle;
  RDMA_PtrType ptr = no_rdma_ptr;
  ByteType num_bytes = no_byte;

  State(
    RDMA_HandleType const& in_handle,
    RDMA_PtrType const& in_ptr = no_rdma_ptr,
    ByteType const& in_num_bytes = no_byte,
    bool const& use_default_handler = false
  );

  template <RDMA_TypeType rdma_type, typename FunctionT>
  RDMA_HandlerType
  set_rdma_fn(
    FunctionT const& fn, bool const& any_tag = false, TagType const& tag = no_tag
  );

  void
  unregister_rdma_handler(
    RDMA_TypeType const& type, TagType const& tag, bool const& use_default
  );

  void
  unregister_rdma_handler(
    RDMA_HandlerType const& handler, TagType const& tag
  );

  RDMA_HandlerType
  make_rdma_handler(RDMA_TypeType const& rdma_type);

  bool
  test_ready_get_data(TagType const& tag);

  bool
  test_ready_put_data(TagType const& tag);

  void
  get_data(
    GetMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info
  );

  void
  put_data(
    PutMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info
  );

  void
  process_pending_get(TagType const& tag = no_tag);

  void
  set_default_handler();

  RDMA_GetType
  default_get_handler_fn(
    BaseMessage* msg, ByteType num_bytes, ByteType req_offset, TagType tag
  );

  void
  default_put_handler_fn(
    BaseMessage* msg, RDMA_PtrType in_ptr, ByteType in_num_bytes,
    ByteType req_offset, TagType tag
  );

  bool using_default_put_handler = false;
  bool using_default_get_handler = false;

  std::unique_ptr<RDMA_GroupType> group_info = nullptr;

private:
  RDMA_HandlerType this_rdma_get_handler = uninitialized_rdma_handler,
                 this_rdma_put_handler = uninitialized_rdma_handler;

  bool get_any_tag = false;
  bool put_any_tag = false;

  RDMA_GetFunctionType rdma_get_fn = no_action;
  RDMA_PutFunctionType rdma_put_fn = no_action;

  tag_container_t<RDMA_TagGetHolderType> get_tag_holder;
  tag_container_t<RDMA_TagPutHolderType> put_tag_holder;

  tag_container_t<container_t<RDMA_InfoType>> pending_tag_gets, pending_tag_puts;
};

template<>
RDMA_HandlerType
State::set_rdma_fn<
  State::RDMA_TypeType::Put, State::RDMA_PutFunctionType
>(RDMA_PutFunctionType const& fn, bool const& any_tag, TagType const& tag);

template<>
RDMA_HandlerType
State::set_rdma_fn<
  State::RDMA_TypeType::Get, State::RDMA_GetFunctionType
>(RDMA_GetFunctionType const& fn, bool const& any_tag, TagType const& tag);


}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMASTATE__*/
