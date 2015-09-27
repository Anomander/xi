#include "xi/ext/type_traits.h"

#include <gtest/gtest.h>

using namespace xi;

TEST(address_of, unique_ptr) {
  unique_ptr< int > uint;
  ASSERT_EQ(uint.get(), address_of(uint));
}

TEST(address_of, shared_ptr) {
  shared_ptr< int > sint;
  ASSERT_EQ(sint.get(), address_of(sint));
}
