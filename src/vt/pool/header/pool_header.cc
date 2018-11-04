
#include "vt/config.h"
#include "vt/pool/header/pool_header.h"
#include "vt/context/context.h"

namespace vt { namespace pool {

/*static*/ char* HeaderManager::setHeader(
  size_t const& num_bytes, size_t const& oversize, char* buffer
) {
  AllocView view;
  view.buffer = buffer;
  view.layout->prealloc.alloc_size = num_bytes;
  view.layout->prealloc.oversize = oversize;
  view.layout->prealloc.alloc_worker = theContext()->getWorker();
  auto buf_start = buffer + sizeof(Header);
  return buf_start;
}

/*static*/ size_t HeaderManager::getHeaderBytes(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.alloc_size;
}

/*static*/ size_t HeaderManager::getHeaderOversizeBytes(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.oversize;
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
