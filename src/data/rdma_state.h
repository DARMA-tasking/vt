
#if ! defined __RUNTIME_TRANSPORT_RDMASTATE__
#define __RUNTIME_TRANSPORT_RDMASTATE__

#include "common.h"
#include "rdma_common.h"
#include "rdma_handle.h"
#include "rdma_msg.h"

#include <unordered_map>
#include <vector>

namespace runtime { namespace rdma {

static constexpr rdma_handler_t const first_rdma_handler = 1;

struct Info {
  using rdma_type_t = Type;

  byte_t num_bytes = no_byte;
  tag_t tag = no_tag;
  rdma_type_t rdma_type;
  rdma_ptr_t data_ptr = no_rdma_ptr;
  rdma_continuation_t cont = no_action;
  action_t cont_action = no_action;

  Info(
    rdma_type_t const& in_rdma_type, byte_t const& in_num_bytes = no_byte,
    tag_t const& in_tag = no_tag, rdma_continuation_t in_cont = no_action,
    action_t in_cont_action = no_action, rdma_ptr_t const& in_data_ptr = no_rdma_ptr
  ) : rdma_type(in_rdma_type), num_bytes(in_num_bytes), tag(in_tag),
      data_ptr(in_data_ptr), cont(in_cont), cont_action(in_cont_action)
  { }
};

struct State {
  using rdma_info_t = Info;
  using rdma_type_t = Type;
  using rdma_get_function_t = active_get_function_t;
  using rdma_put_function_t = active_put_function_t;
  using rdma_tag_get_holder_t = std::tuple<rdma_get_function_t, handler_t>;
  using rdma_tag_put_holder_t = std::tuple<rdma_put_function_t, handler_t>;

  template <typename T>
  using tag_container_t = std::unordered_map<tag_t, T>;

  template <typename T>
  using container_t = std::vector<T>;

  rdma_handle_t handle = no_rdma_handle;
  rdma_ptr_t ptr = no_rdma_ptr;
  byte_t num_bytes = no_byte;

  State(
    rdma_handle_t const& in_handle,
    rdma_ptr_t const& in_ptr = no_rdma_ptr,
    byte_t const& in_num_bytes = no_byte,
    bool const& use_default_handler = false
  );

  template <rdma_type_t rdma_type, typename FunctionT>
  rdma_handler_t
  set_rdma_fn(
    FunctionT const& fn, bool const& any_tag = false, tag_t const& tag = no_tag
  );

  void
  unregister_rdma_handler(
    rdma_type_t const& type, tag_t const& tag, bool const& use_default
  );

  void
  unregister_rdma_handler(
    rdma_handler_t const& handler, tag_t const& tag
  );

  rdma_handler_t
  make_rdma_handler(rdma_type_t const& rdma_type);

  bool
  test_ready_get_data(tag_t const& tag);

  bool
  test_ready_put_data(tag_t const& tag);

  void
  get_data(
    GetMessage* msg, bool const& is_user_msg, rdma_info_t const& info
  );

  void
  put_data(
    PutMessage* msg, bool const& is_user_msg, rdma_info_t const& info
  );

  void
  process_pending_get(tag_t const& tag = no_tag);

  void
  set_default_handler();

  rdma_get_t
  default_get_handler_fn(
    BaseMessage* msg, byte_t num_bytes, tag_t tag
  );

  void
  default_put_handler_fn(
    BaseMessage* msg, rdma_ptr_t in_ptr, byte_t in_num_bytes, tag_t tag
  );

  bool using_default_put_handler = false;
  bool using_default_get_handler = false;

private:
  rdma_handler_t this_rdma_get_handler = uninitialized_rdma_handler,
                 this_rdma_put_handler = uninitialized_rdma_handler;

  bool get_any_tag = false;
  bool put_any_tag = false;

  rdma_get_function_t rdma_get_fn = no_action;
  rdma_put_function_t rdma_put_fn = no_action;

  tag_container_t<rdma_tag_get_holder_t> get_tag_holder;
  tag_container_t<rdma_tag_put_holder_t> put_tag_holder;

  tag_container_t<container_t<rdma_info_t>> pending_tag_gets, pending_tag_puts;
};

template<>
rdma_handler_t
State::set_rdma_fn<
  State::rdma_type_t::Put, State::rdma_put_function_t
>(rdma_put_function_t const& fn, bool const& any_tag, tag_t const& tag);

template<>
rdma_handler_t
State::set_rdma_fn<
  State::rdma_type_t::Get, State::rdma_get_function_t
>(rdma_get_function_t const& fn, bool const& any_tag, tag_t const& tag);


}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMASTATE__*/
