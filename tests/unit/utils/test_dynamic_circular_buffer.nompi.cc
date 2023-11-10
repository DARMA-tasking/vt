#include <gtest/gtest.h>

#include <vt/utils/container/dynamic_circular_buffer.h>
#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

using TestDynamicCircularBuffer = TestHarness;

struct Dummy {
    int x;
};

void validatePresentPhases(util::container::DynamicCircularBuffer<Dummy>& buffer, std::vector<PhaseType> expected) {
    for (auto&& phase : expected) {
        EXPECT_TRUE(buffer.contains(phase));
        EXPECT_EQ(phase * phase, buffer[phase]->x);
    }
}

void validateMissingPhases(util::container::DynamicCircularBuffer<Dummy>& buffer, std::vector<PhaseType> expected) {
    for (auto&& phase : expected) {
        EXPECT_FALSE(buffer.contains(phase));
    }
}

TEST_F(TestDynamicCircularBuffer, test_dynamic_circular_buffer_empty) {
    util::container::DynamicCircularBuffer<Dummy> buffer{10};

    EXPECT_FALSE(buffer.contains(3));
    EXPECT_FALSE(buffer.contains(10));
    
    buffer.resize(2);

    EXPECT_FALSE(buffer.contains(3));
    EXPECT_FALSE(buffer.contains(10));

    buffer.clear();

    EXPECT_FALSE(buffer.contains(3));
    EXPECT_FALSE(buffer.contains(10));
}

TEST_F(TestDynamicCircularBuffer, test_dynamic_circular_buffer_store) {
    util::container::DynamicCircularBuffer<Dummy> buffer{10};

    buffer.store(2, { 2 * 2 });
    validatePresentPhases(buffer, {2});

    // store series of elements
    for (int i = 0; i < 15; i++) {
        buffer.store(i, { i * i });
    }
    validatePresentPhases(buffer, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14});
}

TEST_F(TestDynamicCircularBuffer, test_dynamic_circular_buffer_resize_continuous) {
    util::container::DynamicCircularBuffer<Dummy> buffer{1};

    buffer.store(0, { 0 });
    validatePresentPhases(buffer, {0});

    buffer.resize(10);
    validatePresentPhases(buffer, {0});

    for (int i = 0; i <= 15; i++) {
        buffer.store(i, { i * i });
    }
    validatePresentPhases(buffer, {6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

    buffer.resize(5);
    validatePresentPhases(buffer, {11, 12, 13, 14, 15});
    validateMissingPhases(buffer, {6, 7, 8, 9, 10});

    for (int i = 15; i <= 32; i++) {
        buffer.store(i, { i * i });
    }
    validatePresentPhases(buffer, {28, 29, 30, 31, 32});

    buffer.resize(9);

    for (int i = 33; i <= 35; i++) {
        buffer.store(i, { i * i });
    }
    validatePresentPhases(buffer, {28, 29, 30, 31, 32, 33, 34, 35});

    buffer.resize(1);
    validatePresentPhases(buffer, {35});
}

}}} /* end namespace vt::tests::unit */
