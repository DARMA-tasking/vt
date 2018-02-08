
#include "config.h"
#include "pool/header/pool_header.h"
#include "context/context.h"

namespace vt { namespace pool {

/*static*/ char* HeaderManager::setHeader(size_t const& num_bytes, char* buffer) {
  AllocView view;
  view.buffer = buffer;
  view.layout->prealloc.alloc_size = num_bytes;
  view.layout->prealloc.alloc_worker = theContext()->getWorker();
  auto buf_start = buffer + sizeof(Header);
  return buf_start;
}

/*static*/ size_t HeaderManager::getHeaderBytes(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.alloc_size;
}

/*static*/ WorkerIDType HeaderManager::getHeaderWorker(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.alloc_worker;
}

/*static*/ char* HeaderManager::getHeaderPtr(char* buffer) {
  return buffer - sizeof(Header);
}

}} /* end namespace vt::pool */
