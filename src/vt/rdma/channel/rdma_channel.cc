
#include "vt/rdma/channel/rdma_channel.h"

#define PRINT_RDMA_OP_TYPE(OP) ((OP) == RDMA_TypeType::Get ? "GET" : "PUT")

namespace vt { namespace rdma {

Channel::Channel(
  RDMA_HandleType const& in_rdma_handle, RDMA_TypeType const& in_op_type,
  NodeType const& in_target, TagType const& in_channel_group_tag,
  NodeType const& in_non_target, RDMA_PtrType const& in_ptr, ByteType const& in_num_bytes
) : rdma_handle_(in_rdma_handle), target_(in_target),
    non_target_(in_non_target), num_bytes_(in_num_bytes), ptr_(in_ptr),
    op_type_(in_op_type), channel_group_tag_(in_channel_group_tag)
{
  auto const& my_node = theContext()->getNode();

  is_target_ = target_ == my_node;

  vtAssertExpr(target_ != non_target_);

  vtAssert(
    non_target_ != uninitialized_destination and
    target_ != uninitialized_destination,
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
    "channel: construct: target={}, non_target={}, my_node={}, han={}, "
    "ptr={}, bytes={}, is_target={}\n",
    target_, non_target_, my_node, rdma_handle_, ptr_, num_bytes_,
    print_bool(is_target_)
  );
}

void
Channel::initChannelGroup() {
  debug_print(
    rdma_channel, node,
    "channel: initChannelGroup: target={}, non_target={}, my_node={}, han={}\n",
    target_, non_target_, my_node_, rdma_handle_
  );

  MPI_Group world;
  auto const& group_create_ret = MPI_Comm_group(theContext()->getComm(), &world);

  vtAssert(
    group_create_ret == MPI_SUCCESS,
    "MPI_Comm_group: Should be successful"
  );

  int channel_group_nodes[2] = {};

  channel_group_nodes[target_pos_] = target_;
  channel_group_nodes[non_target_pos_] = non_target_;

  auto const& group_incl_ret = MPI_Group_incl(
    world, 2, channel_group_nodes, &channel_group_
  );

  vtAssert(
    group_incl_ret == MPI_SUCCESS, "MPI_Group_incl: Should be successful"
  );

  auto const& comm_create_ret = MPI_Comm_create_group(
    theContext()->getComm(), channel_group_, channel_group_tag_, &channel_comm_
  );

  vtAssert(
    comm_create_ret == MPI_SUCCESS,
    "MPI_Comm_create_group: Should be successful"
  );

  debug_print(
    rdma_channel, node,
    "channel: initChannelGroup: finished\n"
  );

  initChannelWindow();
}

void
Channel::syncChannelLocal() {
  debug_print(
    rdma_channel, node,
    "channel: syncChannelLocal: target={}, locked={} starting\n",
    target_, print_bool(locked_)
  );

  if (not locked_) {
    lockChannelForOp();
  }

  auto const& ret = MPI_Win_flush_local(non_target_pos_, window_);
  vtAssert(ret == MPI_SUCCESS, "MPI_Win_flush_local: Should be successful");

  if (op_type_ == RDMA_TypeType::Put) {
    unlockChannelForOp();
  }

  debug_print(
    rdma_channel, node,
    "channel: syncChannelLocal: target={}, locked={} finished\n",
    target_, print_bool(locked_)
  );
}

void
Channel::syncChannelGlobal() {
  debug_print(
    rdma_channel, node,
    "channel: syncChannelGlobal: target={} starting\n", target_
  );

  auto const& ret = MPI_Win_flush(target_pos_, window_);
  vtAssert(ret == MPI_SUCCESS, "MPI_Win_flush: Should be successful");

  if (op_type_ == RDMA_TypeType::Put) {
    unlockChannelForOp();
  }

  debug_print(
    rdma_channel, node,
    "channel: syncChannelGlobal: target={} finished\n", target_
  );
}

void
Channel::lockChannelForOp() {
  vtAssert(initialized_, "Channel must be initialized");

  if (not locked_) {
    constexpr int const mpi_win_lock_assert_arg = 0;

    auto const& lock_type =
      (not is_target_) ?
      (op_type_ == RDMA_TypeType::Put ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED) :
      (op_type_ == RDMA_TypeType::Put ? MPI_LOCK_SHARED : MPI_LOCK_SHARED);

    debug_print(
      rdma_channel, node,
      "lockChannelForOp: is_target={}, target={}, op_type={}, "
      "lock_type={}, exclusive={}\n",
      print_bool(is_target_), target_, PRINT_RDMA_OP_TYPE(op_type_),
      lock_type, MPI_LOCK_EXCLUSIVE
    );

    auto const ret = MPI_Win_lock(
      lock_type, target_pos_, mpi_win_lock_assert_arg, window_
    );

    vtAssert(ret == MPI_SUCCESS, "MPI_Win_lock: Should be successful");

    locked_ = true;
  }
}

void
Channel::unlockChannelForOp() {
  vtAssert(initialized_, "Channel must be initialized");

  if (locked_) {
    auto const& ret = MPI_Win_unlock(target_pos_, window_);

    vtAssert(ret == MPI_SUCCESS, "MPI_Win_unlock: Should be successful");

    debug_print(
      rdma_channel, node,
      "unlockChannelForOp: target={}, op_type={}\n",
      target_, PRINT_RDMA_OP_TYPE(op_type_)
    );

    locked_ = false;
  }
}

void
Channel::writeDataToChannel(
  RDMA_PtrType const& ptr, ByteType const& ptr_num_bytes, ByteType const& offset
) {
  vtAssert(initialized_, "Channel must be initialized");
  vtAssert(not is_target_, "The target can not write to this channel");

  ByteType const d_offset = offset == no_byte ? 0 : offset;

  debug_print(
    rdma_channel, node,
    "writeDataToChannel: target={}, ptr={}, ptr_num_bytes={}, "
    "num_bytes={}, op_type={}, offset={}\n",
    target_, ptr, ptr_num_bytes, num_bytes_, PRINT_RDMA_OP_TYPE(op_type_), offset
  );

  if (not locked_) {
    lockChannelForOp();
  }

  if (op_type_ == RDMA_TypeType::Get) {
    auto const& get_ret = MPI_Get(
      ptr, ptr_num_bytes, MPI_BYTE, target_pos_, d_offset, num_bytes_,
      MPI_BYTE, window_
    );
    vtAssert(get_ret == MPI_SUCCESS, "MPI_Get: Should be successful");
  } else if (op_type_ == RDMA_TypeType::Put) {
    auto const& put_ret = MPI_Put(
      ptr, ptr_num_bytes, MPI_BYTE, target_pos_, d_offset, num_bytes_,
      MPI_BYTE, window_
    );
    vtAssert(put_ret == MPI_SUCCESS, "MPI_Put: Should be successful");
  } else {
    vtAssert(0, "op_type must be Get or Put");
  }
}

void
Channel::freeChannel() {
  if (locked_) {
    unlockChannelForOp();
  }
  if (initialized_) {
    MPI_Win_free(&window_);
    MPI_Group_free(&channel_group_);
    MPI_Comm_free(&channel_comm_);
  }
}

/*virtual*/ Channel::~Channel() {
  freeChannel();
}

void
Channel::initChannelWindow() {
  debug_print(
    rdma_channel, node,
    "channel: create window: num_bytes={}\n", num_bytes_
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

  vtAssert(
    win_create_ret == MPI_SUCCESS, "MPI_Win_create: Should be successful"
  );

  initialized_ = true;

  debug_print(
    rdma_channel, node,
    "channel: initChannel: finished creating window\n"
  );
}

NodeType
Channel::getTarget() const {
  return target_;
}

NodeType
Channel::getNonTarget() const {
  return non_target_;
}

}} //end namespace vt::rdma

#undef PRINT_RDMA_OP_TYPE
