#if ! defined __VIRTUAL_TRANSPORT_TEST_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_HARNESS__

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>

#include "test_config.h"
#include "data_message.h"

namespace vt { namespace tests { namespace unit {

extern int test_argc;
extern char** test_argv;

template <typename TestBase>
struct TestHarnessAny : TestBase {
  virtual void SetUp() {
    argc_ = orig_args_.size();
    argv_ = new char*[argc_];

    for (int i = 0; i < argc_; i++) {
      auto const len = orig_args_[i].size();
      argv_[i] = new char[len + 1];
      argv_[i][len] = 0;
      orig_args_[i].copy(argv_[i], len);
    }
  }

  virtual void TearDown() {
    for (int i = 0; i < argc_; i++) {
      delete [] argv_[i];
    }
    delete [] argv_;
  }

  static void store_cmdline_args(int argc, char **argv) {
    orig_args_ = std::vector<std::string>(argv, argv + argc);
  }

  static std::vector<std::string> orig_args_;

  int argc_ = 0;
  char** argv_ = nullptr;
};

template <typename TestBase>
std::vector<std::string> TestHarnessAny<TestBase>::orig_args_;

using TestHarness = TestHarnessAny<testing::Test>;

template <typename ParamT>
using TestHarnessParam = TestHarnessAny<testing::TestWithParam<ParamT>>;

}}} // end namespace vt::tests::unit

#endif /* __VIRTUAL_TRANSPORT_TEST_HARNESS__ */
