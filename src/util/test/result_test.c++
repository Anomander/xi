#include "xi/ext/configure.h"
#include "xi/util/result.h"

#include <gtest/gtest.h>

using namespace xi::prelude;

enum error2 {};
enum class errors {
  ERROR_ONE = 1,
  ERROR_TWO = 2,
};

class Foo {};

XI_DECLARE_ERROR_ENUM(errors);

TEST(interface, empty_construction_does_nothing) {
  result<> r;
  r.to_error();
}
