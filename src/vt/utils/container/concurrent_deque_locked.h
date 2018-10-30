
#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_H

#include "config.h"
#include "context/context.h"
#include "utils/mutex/mutex.h"

#include <deque>
#include <mutex>
#include <functional>
#include <cassert>

namespace vt { namespace util { namespace container {

using ::vt::util::mutex::MutexType;

/*
 * Implement a very simple concurrent deque that just uses std::mutex for access
 * control... obviously this should be greatly improved and made lock free but
 * this enables programming to a consistent interface
 */

template <typename T>
struct ConcurrentDequeLocked {
  using ContainerType = std::deque<T>;
  using SizeType = typename ContainerType::size_type;
  using TConstRef = typename ContainerType::const_reference;
  using TRef = typename ContainerType::reference;

  ConcurrentDequeLocked()
    : ConcurrentDequeLocked(theContext() ? theContext()->hasWorkers() : true)
  { }
  explicit ConcurrentDequeLocked(bool in_needs_lock)
    : needs_lock_(in_needs_lock)
  { }
  ConcurrentDequeLocked(ConcurrentDequeLocked const&) = delete;

  virtual ~ConcurrentDequeLocked() { }

  void emplaceBack(T&& elm);
  void emplaceFront(T&& elm);
  void pushBack(T const& elm);
  void pushFront(T const& elm);

  TConstRef front() const;
  TConstRef back() const;

  T popGetFront();
  T popGetBack();

  TRef at(SizeType const& pos);
  TConstRef at(SizeType const& pos) const;

  TRef front();
  TRef back();

  void popFront();
  void popBack();

  SizeType size();

private:
  MutexType* getMutex();

private:
  bool needs_lock_ = true;
  MutexType container_mutex_{};
  ContainerType container_;
};

}}} //end namespace vt::util::container

#include "concurrent_deque_locked.impl.h"

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_H*/

