#include "xi/util/option.h"

#include <gtest/gtest.h>

using namespace xi;

struct test_shared : public virtual ownership::std_shared {
  int value = 42;
  test_shared() = default;
  test_shared(int v) : value(v) {}
  test_shared(test_shared const &) = delete;
};
struct test_unique : public virtual ownership::unique {
  int value = 42;
  test_unique() = default;
  test_unique(int v) : value(v) {}
  test_unique(test_unique const &) = delete;
};

template < class > class P;

TEST(simple, is_none) {
  option< int > surely_int = some(42);
  option< int > not_int = none;

  ASSERT_TRUE(surely_int.is_some());
  ASSERT_FALSE(surely_int.is_none());

  ASSERT_FALSE(not_int.is_some());
  ASSERT_TRUE(not_int.is_none());
}

TEST(simple, as_mut) {
  option< int > maybe_int = some(42);
  option< own< test_shared > > maybe_shared = some(make< test_shared >());
  option< own< test_unique > > maybe_unique = some(make< test_unique >());

  STATIC_ASSERT_TEST(is_same< option< int * >, decltype(maybe_int.as_mut()) >);
  STATIC_ASSERT_TEST(
      is_same< option< mut< test_shared > >, decltype(maybe_shared.as_mut()) >);
  STATIC_ASSERT_TEST(
      is_same< option< mut< test_unique > >, decltype(maybe_unique.as_mut()) >);

  *(maybe_int.as_mut().unwrap()) = 0;
  ASSERT_EQ(*maybe_int.as_mut().unwrap(), 0);

  auto mut_shared = maybe_shared.as_mut().unwrap();
  mut_shared->value = 0;
  ASSERT_EQ(address_of(maybe_shared.as_mut().unwrap()), address_of(mut_shared));

  auto mut_unique = maybe_unique.as_mut().unwrap();
  mut_unique->value = 0;
  ASSERT_EQ(address_of(maybe_unique.as_mut().unwrap()), address_of(mut_unique));
}

TEST(simple, as_ref) {
  option< int > maybe_int = some(42);
  option< own< test_shared > > maybe_shared = some(make< test_shared >());
  option< own< test_unique > > maybe_unique = some(make< test_unique >());

  STATIC_ASSERT_TEST(
      is_same< option< int const & >, decltype(maybe_int.as_ref()) >);
  STATIC_ASSERT_TEST(
      is_same< option< ref< test_shared > >, decltype(maybe_shared.as_ref()) >);
  STATIC_ASSERT_TEST(
      is_same< option< ref< test_unique > >, decltype(maybe_unique.as_ref()) >);

  auto &ref_shared = maybe_shared.as_ref().unwrap();
  ASSERT_EQ(address_of(maybe_shared.as_ref().unwrap()), address_of(ref_shared));

  auto &ref_unique = maybe_unique.as_ref().unwrap();
  ASSERT_EQ(address_of(maybe_unique.as_ref().unwrap()), address_of(ref_unique));
}

TEST(simple, unwrap) {
  option< int > maybe_int = some(42);
  option< own< test_shared > > maybe_shared = some(make< test_shared >());
  option< own< test_unique > > maybe_unique = some(make< test_unique >());

  STATIC_ASSERT_TEST(is_same< int, decltype(maybe_int.unwrap()) >);
  STATIC_ASSERT_TEST(
      is_same< own< test_shared >, decltype(maybe_shared.unwrap()) >);
  STATIC_ASSERT_TEST(
      is_same< own< test_unique >, decltype(maybe_unique.unwrap()) >);

  auto shared = maybe_shared.unwrap();
  ASSERT_FALSE(is_shared(shared)); // this is the only reference
  ASSERT_EQ(42, shared->value);
  ASSERT_TRUE(maybe_shared.is_none());

  auto unique = maybe_unique.unwrap();
  ASSERT_EQ(42, unique->value);
  ASSERT_TRUE(maybe_unique.is_none());
}

TEST(simple, unwrap_or) {
  option< int > maybe_int = none;
  option< own< test_shared > > maybe_shared = none;
  option< own< test_unique > > maybe_unique = none;

  STATIC_ASSERT_TEST(is_same< int, decltype(maybe_int.unwrap_or(42)) >);
  STATIC_ASSERT_TEST(
      is_same< own< test_shared >,
               decltype(maybe_shared.unwrap_or(make< test_shared >())) >);
  STATIC_ASSERT_TEST(
      is_same< own< test_unique >,
               decltype(maybe_unique.unwrap_or(make< test_unique >())) >);

  ASSERT_TRUE(maybe_shared.is_none());
  auto shared = maybe_shared.unwrap_or(make< test_shared >());
  ASSERT_EQ(42, shared->value);
  ASSERT_TRUE(maybe_shared.is_none());
  ASSERT_FALSE(is_shared(shared)); // this is the only reference

  ASSERT_TRUE(maybe_unique.is_none());
  auto unique = maybe_unique.unwrap_or(make< test_unique >());
  ASSERT_EQ(42, unique->value);
  ASSERT_TRUE(maybe_unique.is_none());
}

TEST(simple, unwrap_or_func) {
  option< int > maybe_int = none;
  option< own< test_shared > > maybe_shared = none;
  option< own< test_unique > > maybe_unique = none;

  ASSERT_TRUE(maybe_shared.is_none());
  auto shared = maybe_shared.unwrap_or([] { return make< test_shared >(); });
  STATIC_ASSERT_TEST(is_same< own< test_shared >, decltype(shared) >);
  ASSERT_EQ(42, shared->value);
  ASSERT_TRUE(maybe_shared.is_none());
  ASSERT_FALSE(is_shared(shared)); // this is the only reference

  ASSERT_TRUE(maybe_unique.is_none());
  auto unique = maybe_unique.unwrap_or([] { return make< test_unique >(); });
  STATIC_ASSERT_TEST(is_same< own< test_unique >, decltype(unique) >);
  ASSERT_EQ(42, unique->value);
  ASSERT_TRUE(maybe_unique.is_none());
}

TEST(simple, map_value) {
  option< int > surely_int = some(42);
  option< own< test_shared > > surely_shared = some(make< test_shared >());
  option< own< test_unique > > surely_unique = some(make< test_unique >());

  auto number = surely_int.map([](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(number) >);
  ASSERT_EQ(84, number.unwrap());
  ASSERT_TRUE(surely_int.is_none());

  auto shared = surely_shared.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(shared) >);
  ASSERT_EQ(84, shared.unwrap());
  ASSERT_TRUE(surely_shared.is_none());

  auto unique = surely_unique.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(unique) >);
  ASSERT_EQ(84, unique.unwrap());
  ASSERT_TRUE(surely_unique.is_none());
}

TEST(simple, map_none) {
  option< int > not_int = none;
  option< own< test_shared > > not_shared = none;
  option< own< test_unique > > not_unique = none;

  auto number = not_int.map([](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(number) >);
  ASSERT_TRUE(number.is_none());
  ASSERT_TRUE(not_int.is_none());

  auto shared = not_shared.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(shared) >);
  ASSERT_TRUE(shared.is_none());
  ASSERT_TRUE(not_shared.is_none());

  auto unique = not_unique.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(unique) >);
  ASSERT_TRUE(unique.is_none());
  ASSERT_TRUE(not_unique.is_none());
}

TEST(simple, map_or_value) {
  option< int > not_int = none;
  option< own< test_shared > > not_shared = none;
  option< own< test_unique > > not_unique = none;

  auto number = not_int.map_or(21, [](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(number) >);
  ASSERT_EQ(21, number.unwrap());
  ASSERT_TRUE(not_int.is_none());

  auto shared = not_shared.map_or(21, [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(shared) >);
  ASSERT_EQ(21, shared.unwrap());
  ASSERT_TRUE(not_shared.is_none());

  auto unique = not_unique.map_or(21, [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(unique) >);
  ASSERT_EQ(21, unique.unwrap());
  ASSERT_TRUE(not_unique.is_none());
}

TEST(simple, map_or_func) {
  option< int > not_int = none;
  option< own< test_shared > > not_shared = none;
  option< own< test_unique > > not_unique = none;

  auto number =
      not_int.map_or([] { return 1; }, [](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(number) >);
  ASSERT_EQ(1, number.unwrap());
  ASSERT_TRUE(not_int.is_none());

  auto shared = not_shared.map_or([] { return 1; },
                                  [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(shared) >);
  ASSERT_EQ(1, shared.unwrap());
  ASSERT_TRUE(not_shared.is_none());

  auto unique = not_unique.map_or([] { return 1; },
                                  [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(unique) >);
  ASSERT_EQ(1, unique.unwrap());
  ASSERT_TRUE(not_unique.is_none());
}

TEST(simple, and_positive) {
  option< int > surely_int = some(42);
  option< own< test_shared > > surely_shared = some(make< test_shared >());
  option< own< test_unique > > surely_unique = some(make< test_unique >());

  auto number = surely_int.and_(some(21));
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(number) >);
  ASSERT_EQ(21, number.unwrap());
  ASSERT_TRUE(surely_int.is_some());

  auto shared = surely_shared.and_(some(21));
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(shared) >);
  ASSERT_EQ(21, shared.unwrap());
  ASSERT_TRUE(surely_shared.is_some());

  auto unique = surely_unique.and_(some(21));
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(unique) >);
  ASSERT_EQ(21, unique.unwrap());
  ASSERT_TRUE(surely_unique.is_some());
}

TEST(simple, and_negative) {
  option< int > surely_int = none;
  option< own< test_shared > > surely_shared = none;
  option< own< test_unique > > surely_unique = none;

  auto number = surely_int.and_(some(21));
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(number) >);
  ASSERT_TRUE(number.is_none());

  auto shared = surely_shared.and_(some(21));
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(shared) >);
  ASSERT_TRUE(shared.is_none());

  auto unique = surely_unique.and_(some(21));
  STATIC_ASSERT_TEST(is_same< option< int >, decltype(unique) >);
  ASSERT_TRUE(unique.is_none());
}

TEST(simple, comparison) {
  auto surely_int = some(42);
  option< int > not_int = none;

  ASSERT_FALSE(not_int == surely_int);
  ASSERT_TRUE(not_int == none);
  ASSERT_TRUE(surely_int == some(42));
  ASSERT_FALSE(surely_int != some(42));
  ASSERT_FALSE(surely_int == some(21));
  ASSERT_TRUE(surely_int != some(21));
  ASSERT_FALSE(surely_int == none);
  ASSERT_TRUE(surely_int != none);
  ASSERT_TRUE(surely_int > some(21));
  ASSERT_FALSE(surely_int < some(21));
}