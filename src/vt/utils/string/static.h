
#if !defined INCLUDED_UTILS_STRING_STATIC_H
#define INCLUDED_UTILS_STRING_STATIC_H

#include "config.h"

#include <cstring>

namespace vt { namespace util { namespace string {

struct StatStr {
  using IteratorConstType =  char const*;

  template <std::size_t N>
  constexpr StatStr(char const(&a)[N]           ) noexcept : p_(a),sz_(N-1) { }
  constexpr StatStr(char const* p, std::size_t N) noexcept : p_(p),sz_(N)   { }

  constexpr char const*       data()       const noexcept { return p_;       }
  constexpr std::size_t       size()       const noexcept { return sz_;      }
  constexpr IteratorConstType begin()      const noexcept { return p_;       }
  constexpr IteratorConstType end()        const noexcept { return p_ + sz_; }
  constexpr char operator[](std::size_t n) const noexcept { return  p_[n];   }

private:
  char const* const p_ = nullptr;
  std::size_t const sz_ = 0;
};

}}} /* end namespace vt::util::string */

#endif /*INCLUDED_UTILS_STRING_STATIC_H*/
