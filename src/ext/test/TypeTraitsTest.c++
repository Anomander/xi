#include "xi/util/PolymorphicSpScRingBuffer.h"

#include <gtest/gtest.h>

using namespace xi;

TEST(AddressOf, UniquePtr) {
  unique_ptr <int> uint;
  ASSERT_EQ (uint.get(), addressOf(uint));
}

TEST(AddressOf, SharedPtr) {
  shared_ptr <int> sint;
  ASSERT_EQ (sint.get(), addressOf(sint));
}
