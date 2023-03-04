#if !defined INCLUDED_PERF_COMMON_TEST_HARNESS_BASE_H
#define INCLUDED_PERF_COMMON_TEST_HARNESS_BASE_H

#include "timers.h"

#include <cstdint>
#include <string>

namespace vt::tests::perf::common {

struct TestHarnessBase {
  using TestName = std::string;
  using TestResult = std::pair<TestName, TimeType>;

  virtual void SetUp() = 0;
  virtual void TearDown() = 0;
  virtual std::string GetName() const = 0;
  virtual uint32_t GetNumRuns() const = 0;
  virtual void TestFunc() {}
  virtual void AddResult(TestResult const& test_result) = 0;
  virtual void DumpResults() = 0;
  virtual void SyncResults() = 0;

  virtual ~TestHarnessBase() = default;
};

} // namespace vt::tests::perf::common

#endif