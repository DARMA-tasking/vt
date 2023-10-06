
#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

namespace vt {

namespace CLI {
class App;
} /* end namespace CLI */

namespace arguments {

struct ArgvContainer {
  ArgvContainer(int& argc, char**& argv)
    : argv_vector_(argv, argv + argc)
   {}

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | argv_vector_;
  }

  std::vector<std::string> argv_vector_;
};

} // namespace arguments
} // namespace vt

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H*/