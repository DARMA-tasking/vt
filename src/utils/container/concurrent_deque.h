
#if ! defined __RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE__
#define __RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE__

#include "config.h"

#include <deque>
#include <mutex>
#include <cassert>

namespace vt { namespace util { namespace container {

/*
 * Implement a very simple concurrent deque that just uses std::mutex for access
 * control... obviously this should be greatly improved and made lock free but
 * this enables programming to a consistent interface
 */

template <typename T>
struct ConcurrentDeque {
  using ContainerType = std::deque<T>;
  using SizeType = typename ContainerType::size_type;
  using TConstRef = typename ContainerType::const_reference;
  using TRef = typename ContainerType::reference;

  ConcurrentDeque() = default;
  ConcurrentDeque(ConcurrentDeque const&) = delete;

  virtual ~ConcurrentDeque() { }

  void emplaceBack(T&& elm);
  void emplaceFront(T&& elm);
  void pushBack(T const& elm);
  void pushFront(T const& elm);

  TConstRef front() const;
  TConstRef back() const;

  TRef at(SizeType const& pos);
  TConstRef at(SizeType const& pos) const;

  TRef front();
  TRef back();

  void popFront();
  void popBack();

  SizeType size();

private:
  std::mutex container_mutex_{};

  ContainerType container_;
};

}}} //end namespace vt::util::container

#include "concurrent_deque.impl.h"

#endif /*__RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE__*/

