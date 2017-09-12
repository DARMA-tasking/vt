
#include "rdma_channel.h"

namespace vt { namespace rdma {

Channel::Channel(
  RDMA_HandleType const& in_rdma_handle, RDMA_TypeType const& in_op_type,
  NodeType const& in_target, TagType const& in_channel_group_tag,
  NodeType const& in_non_target, RDMA_PtrType const& in_ptr, ByteType const& in_num_bytes
) : rdma_handle_(in_rdma_handle), target_(in_target),
    non_target_(in_non_target), num_bytes_(in_num_bytes), ptr_(in_ptr),
    op_type_(in_op_type), channel_group_tag_(in_channel_group_tag)
{
  auto const& my_node = theContext->get_node();

  is_target_ = target_ == my_node;

  assert(target_ != non_target_);

  assert(
    non_target_ != uninitialized_destination and
    target_ != uninitialized_destination and
    "Channel must know both target and non_target"
  );

  if (in_num_bytes == no_byte) {
    num_bytes_ = rdma_empty_byte;
  }

  // set positions in local group
  target_pos_ = 0;
  non_target_pos_ = 1;

  debug_print(
    rdma_channel, node,
    "channel: construct: target=%d, non_target=%d, my_node=%d, han=%lld, "
    "ptr=%p, bytes=%lld, is_target=%s\n",
    target, non_target, my_node, rdma_handle, ptr, num_bytes,
    print_bool(is_target)
  );
}

void
Channel::init_channel_group() {
  debug_print(
    rdma_channel, node,
    "channel: init_channel_group: target=%d, non_target=%d, my_node=%d, han=%lld\n",
    target, non_target, my_node, rdma_handle
  );

  MPI_Group world;
  auto const& group_create_ret = MPI_Comm_group(MPI_COMM_WORLD, &world);

  assert(
    group_create_ret == MPI_SUCCESS and
    "MPI_Comm_group: Should be successful"
  );

  int channel_group_nodes[2] = {};

  channel_group_nodes[target_pos_] = target_;
  channel_group_nodes[non_target_pos_] = non_target_;

  auto const& group_incl_ret = MPI_Group_incl(
    world, 2, channel_group_nodes, &channel_group_
  );

  assert(
    group_incl_ret == MPI_SUCCESS and "MPI_Group_incl: Should be successful"
  );

  auto const& comm_create_ret = MPI_Comm_create_group(
    MPI_COMM_WORLD, channel_group_, channel_group_tag_, &channel_comm_
  );

  assert(
    comm_create_ret == MPI_SUCCESS and
    "MPI_Comm_create_group: Should be successful"
  );

  debug_print(
    rdma_channel, node,
    "channel: init_channel_group: finished\n"
  );

  init_channel_window();
}

void
Channel::sync_channel_local() {
  debug_print(
    rdma_channel, node,
    "channel: sync_channel_local: target=%d, locked=%s starting\n",
    target, print_bool(locked)
  );

  if (not locked_) {
    lock_channel_for_op();
  }

  auto const& ret = MPI_Win_flush_local(non_target_pos_, window_);
  assert(ret == MPI_SUCCESS and "MPI_Win_flush_local: Should be successful");

  if (op_type_ == RDMA_TypeType::Put) {
    unlock_channel_for_op();
  }

  debug_print(
    rdma_channel, node,
    "channel: sync_channel_local: target=%d, locked=%s finished\n",
    target, print_bool(locked)
  );
}

void
Channel::sync_channel_global() {
  debug_print(
    rdma_channel, node,
    "channel: sync_channel_global: target=%d starting\n", target
  );

  auto const& ret = MPI_Win_flush(target_pos_, window_);
  assert(ret == MPI_SUCCESS and "MPI_Win_flush: Should be successful");

  if (op_type_ == RDMA_TypeType::Put) {
    unlock_channel_for_op();
  }

  debug_print(
    rdma_channel, node,
    "channel: sync_channel_global: target=%d finished\n", target
  );
}

void
Channel::lock_channel_for_op() {
  assert(initialized_ and "Channel must be initialized");

  if (not locked_) {
    constexpr int const mpi_win_lock_assert_arg = 0;

    auto const& lock_type =
      (not is_target_) ?
      (op_type_ == RDMA_TypeType::Put ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED) :
      (op_type_ == RDMA_TypeType::Put ? MPI_LOCK_SHARED : MPI_LOCK_SHARED);

    debug_print(
      rdma_channel, node,
      "lock_channel_for_op: is_target=%s, target=%d, op_type=%s, "
      "lock_type=%d, exclusive=%d\n",
      print_bool(is_target), target,
      op_type == RDMA_TypeType::Get ? "GET" : "PUT", lock_type, MPI_LOCK_EXCLUSIVE
    );

    auto const ret = MPI_Win_lock(
      lock_type, target_pos_, mpi_win_lock_assert_arg, window_
    );

    assert(ret == MPI_SUCCESS and "MPI_Win_lock: Should be successful");

    locked_ = true;
  }
}

void
Channel::unlock_channel_for_op() {
  assert(initialized_ and "Channel must be initialized");

  if (locked_) {
    auto const& ret = MPI_Win_unlock(target_pos_, window_);

    assert(ret == MPI_SUCCESS and "MPI_Win_unlock: Should be successful");

    debug_print(
      rdma_channel, node,
      "unlock_channel_for_op: target=%d, op_type=%s\n",
      target, op_type == RDMA_TypeType::Get ? "GET" : "PUT"
    );

    locked_ = false;
  }
}

void
Channel::write_data_to_channel(
  RDMA_PtrType const& ptr, ByteType const& ptr_num_bytes, ByteType const& offset
) {
  assert(initialized_ and "Channel must be initialized");
  assert(not is_target_ and "The target can not write to this channel");

  ByteType const d_offset = offset == no_byte ? 0 : offset;

  debug_print(
    rdma_channel, node,
    "write_data_to_channel: target=%d, ptr=%p, ptr_num_bytes=%lld, "
    "num_bytes=%lld, op_type=%s, offset=%lld\n",
    target, ptr, ptr_num_bytes, num_bytes,
    op_type == RDMA_TypeType::Get ? "GET" : "PUT", offset
  );

  if (not locked_) {
    lock_channel_for_op();
  }

  if (op_type_ == RDMA_TypeType::Get) {
    auto const& get_ret = MPI_Get(
      ptr, ptr_num_bytes, MPI_BYTE, target_pos_, d_offset, num_bytes_, MPI_BYTE, window_
    );
    assert(get_ret == MPI_SUCCESS and "MPI_Get: Should be successful");
  } else if (op_type_ == RDMA_TypeType::Put) {
    auto const& put_ret = MPI_Put(
      ptr, ptr_num_bytes, MPI_BYTE, target_pos_, d_offset, num_bytes_, MPI_BYTE, window_
    );
    assert(put_ret == MPI_SUCCESS and "MPI_Put: Should be successful");
  } else {
    assert(0 and "op_type must be Get or Put");
  }
}

void
Channel::free_channel() {
  if (locked_) {
    unlock_channel_for_op();
  }
  if (initialized_) {
    MPI_Win_free(&window_);
    MPI_Group_free(&channel_group_);
    MPI_Comm_free(&channel_comm_);
  }
}

Channel::~Channel() {
  free_channel();
}

void
Channel::init_channel_window() {
  debug_print(
    rdma_channel, node,
    "channel: create window: num_bytes=%lld\n", num_bytes
  );

  int win_create_ret = 0;

  if (is_target_) {
    win_create_ret = MPI_Win_create(
      ptr_, num_bytes_, rdma_elm_size, MPI_INFO_NULL, channel_comm_, &window_
    );
  } else {
    win_create_ret = MPI_Win_create(
      nullptr, 0, 1, MPI_INFO_NULL, channel_comm_, &window_
    );
  }

  assert(
    win_create_ret == MPI_SUCCESS and "MPI_Win_create: Should be successful"
  );

  initialized_ = true;

  debug_print(
    rdma_channel, node,
    "channel: init_channel: finished creating window\n"
  );
}

NodeType
Channel::get_target() const {
  return target_;
}

NodeType
Channel::get_non_target() const {
  return non_target_;
}

}} //end namespace vt::rdma
