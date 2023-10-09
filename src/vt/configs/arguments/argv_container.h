
#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

namespace vt {

namespace arguments {

struct ArgvContainer {
  ArgvContainer(int& argc, char**& argv)
  {
    std::vector<char*> non_vt_args;
    for(int i = 0; i < argc; i++) {
      // cache original argv parameter
      argv_.push_back(strdup(argv[i]));
      // collect non vt params
      if (!((0 == strncmp(argv[i], "--vt_", 5)) ||
          (0 == strncmp(argv[i], "!--vt_", 6)))) {
        non_vt_args.push_back(argv[i]);
      }
    }

    // Reconstruct argv without vt related params
    int new_argc = non_vt_args.size();
    static std::unique_ptr<char*[]> new_argv = nullptr;

    new_argv = std::make_unique<char*[]>(new_argc + 1);

    int i = 0;
    for (auto&& arg : non_vt_args) {
      new_argv[i++] = arg;
    }
    new_argv[i++] = nullptr;

    argc = new_argc;
    argv = new_argv.get();
  }

  ~ArgvContainer() {
    for(char* param: argv_) {
      delete param;
    }
  }

  ArgvContainer(const ArgvContainer&) = delete;
  ArgvContainer& operator=(const ArgvContainer&) = delete;


  std::vector<char*> argv_;
};

} // namespace arguments
} // namespace vt

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H*/