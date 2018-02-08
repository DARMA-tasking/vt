
#include "config.h"
#include "pool/pool.h"
#include "worker/worker_headers.h"
#include "pool/static_sized/memory_pool_equal.h"

#include <cstdlib>
#include <cstdint>

namespace vt { namespace pool {

Pool::Pool()
  : small_msg(initSPool()), medium_msg(initMPool())
{ }

/*static*/Pool::MemPoolSType Pool::initSPool() {
  return std::make_unique<MemoryPoolType<memory_size_small>>();
}

/*static*/ Pool::MemPoolMType Pool::initMPool() {
  return std::make_unique<MemoryPoolType<memory_size_medium>>(64);
}

Pool::ePoolSize Pool::getPoolType(size_t const& num_bytes) {
  if (num_bytes <= small_msg->getNumBytes()) {
    return ePoolSize::Small;
  } else if (num_bytes <= medium_msg->getNumBytes()) {
    return ePoolSize::Medium;
  } else {
    return ePoolSize::Malloc;
  }
}

void* Pool::alloc(size_t const& num_bytes) {
  void* ret = nullptr;

  ePoolSize const pool_type = getPoolType(num_bytes);

  auto const worker = theContext()->getWorker();
  bool const comm_thread = worker == worker_id_comm_thread;

  if (pool_type == ePoolSize::Small) {
    auto pool = comm_thread ? small_msg.get() : s_msg_worker_[worker].get();
    assert(
      (comm_thread || s_msg_worker_.size() > worker) && "Must have worker pool"
    );
    ret = pool->alloc(num_bytes);
  } else if (pool_type == ePoolSize::Medium) {
    auto pool = comm_thread ? medium_msg.get() : m_msg_worker_[worker].get();
    assert(
      (comm_thread || m_msg_worker_.size() > worker) && "Must have worker pool"
    );
    ret = pool->alloc(num_bytes);
  } else {
    auto alloc_buf = std::malloc(num_bytes + sizeof(HeaderType));
    ret = HeaderManagerType::setHeader(num_bytes, static_cast<char*>(alloc_buf));
  }

  debug_print(
    pool, node,
    "Pool::alloc of size=%zu, type=%s, ret=%p, worker=%d\n",
    num_bytes, print_pool_type(pool_type), ret, worker
  );

  return ret;
}

void Pool::dealloc(void* const buf) {
  auto buf_char = static_cast<char*>(buf);
  auto const& actual_alloc_size = HeaderManagerType::getHeaderBytes(buf_char);
  auto const& alloc_worker = HeaderManagerType::getHeaderWorker(buf_char);
  auto const& ptr_actual = HeaderManagerType::getHeaderPtr(buf_char);
  auto const worker = theContext()->getWorker();

  ePoolSize const pool_type = getPoolType(actual_alloc_size);

  debug_print(
    pool, node,
    "Pool::dealloc of buf=%p, type=%s, alloc_size=%ld, worker=%d, ptr=%p\n",
    buf, print_pool_type(pool_type), actual_alloc_size, alloc_worker, ptr_actual
  );

  if (pool_type != ePoolSize::Malloc && alloc_worker != worker) {
    theWorkerGrp()->enqueueForWorker(worker, [buf]{
      thePool()->dealloc(buf);
    });
    return;
  }

  if (pool_type == ePoolSize::Small) {
    small_msg->dealloc(buf);
  } else if (pool_type == ePoolSize::Medium) {
    medium_msg->dealloc(buf);
  } else {
    std::free(ptr_actual);
  }
};

Pool::SizeType Pool::remainingSize(void* const buf) {
  auto buf_char = static_cast<char*>(buf);
  auto const& actual_alloc_size = HeaderManagerType::getHeaderBytes(buf_char);

  ePoolSize const pool_type = getPoolType(actual_alloc_size);

  if (pool_type == ePoolSize::Small) {
    return small_msg->getNumBytes() - actual_alloc_size;
  } else if (pool_type == ePoolSize::Medium) {
    return medium_msg->getNumBytes() - actual_alloc_size;
  } else {
    return 0;
  }
}

void Pool::initWorkerPools(WorkerCountType const& num_workers) {
  for (auto i = 0; i < num_workers; i++) {
    s_msg_worker_.emplace_back(initSPool());
    m_msg_worker_.emplace_back(initMPool());
  }
}

void Pool::destroyWorkerPools() {
  s_msg_worker_.clear();
  m_msg_worker_.clear();
}

}} //end namespace vt::pool
