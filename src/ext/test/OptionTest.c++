#include "xi/util/Option.h"

#include <gtest/gtest.h>

using namespace xi;

struct TestShared : public virtual ownership::StdShared {
  int value = 42;
  TestShared() = default;
  TestShared(int v) : value (v) {}
  TestShared(TestShared const &) = delete;
};
struct TestUnique : public virtual ownership::Unique {
  int value = 42;
  TestUnique() = default;
  TestUnique(int v) : value (v) {}
  TestUnique(TestUnique const &) = delete;
};

template < class >
class P;

TEST(Simple, isNone) {
  Option< int > surelyInt = Some(42);
  Option< int > notInt = None;

  ASSERT_TRUE(surelyInt.isSome());
  ASSERT_FALSE(surelyInt.isNone());

  ASSERT_FALSE(notInt.isSome());
  ASSERT_TRUE(notInt.isNone());
}

TEST(Simple, asMut) {
  Option< int > maybeInt = Some(42);
  Option< own< TestShared > > maybeShared = Some(make< TestShared >());
  Option< own< TestUnique > > maybeUnique = Some(make< TestUnique >());

  STATIC_ASSERT_TEST(is_same< Option< int * >, decltype(maybeInt.asMut()) >);
  STATIC_ASSERT_TEST(is_same< Option< mut< TestShared > >, decltype(maybeShared.asMut()) >);
  STATIC_ASSERT_TEST(is_same< Option< mut< TestUnique > >, decltype(maybeUnique.asMut()) >);

  *(maybeInt.asMut().unwrap()) = 0;
  ASSERT_EQ(*maybeInt.asMut().unwrap(), 0);

  auto mutShared = maybeShared.asMut().unwrap();
  mutShared->value = 0;
  ASSERT_EQ(addressOf(maybeShared.asMut().unwrap()), addressOf(mutShared));

  auto mutUnique = maybeUnique.asMut().unwrap();
  mutUnique->value = 0;
  ASSERT_EQ(addressOf(maybeUnique.asMut().unwrap()), addressOf(mutUnique));
}

TEST(Simple, asRef) {
  Option< int > maybeInt = Some(42);
  Option< own< TestShared > > maybeShared = Some(make< TestShared >());
  Option< own< TestUnique > > maybeUnique = Some(make< TestUnique >());

  STATIC_ASSERT_TEST(is_same< Option< int const & >, decltype(maybeInt.asRef()) >);
  STATIC_ASSERT_TEST(is_same< Option< ref< TestShared > >, decltype(maybeShared.asRef()) >);
  STATIC_ASSERT_TEST(is_same< Option< ref< TestUnique > >, decltype(maybeUnique.asRef()) >);

  auto &refShared = maybeShared.asRef().unwrap();
  ASSERT_EQ(addressOf(maybeShared.asRef().unwrap()), addressOf(refShared));

  auto &refUnique = maybeUnique.asRef().unwrap();
  ASSERT_EQ(addressOf(maybeUnique.asRef().unwrap()), addressOf(refUnique));
}

TEST(Simple, unwrap) {
  Option< int > maybeInt = Some(42);
  Option< own< TestShared > > maybeShared = Some(make< TestShared >());
  Option< own< TestUnique > > maybeUnique = Some(make< TestUnique >());

  STATIC_ASSERT_TEST(is_same< int, decltype(maybeInt.unwrap()) >);
  STATIC_ASSERT_TEST(is_same< own< TestShared >, decltype(maybeShared.unwrap()) >);
  STATIC_ASSERT_TEST(is_same< own< TestUnique >, decltype(maybeUnique.unwrap()) >);

  auto shared = maybeShared.unwrap();
  ASSERT_FALSE(isShared(shared)); // this is the only reference
  ASSERT_EQ(42, shared->value);
  ASSERT_TRUE(maybeShared.isNone());

  auto unique = maybeUnique.unwrap();
  ASSERT_EQ(42, unique->value);
  ASSERT_TRUE(maybeUnique.isNone());
}

TEST(Simple, unwrapOr) {
  Option< int > maybeInt = None;
  Option< own< TestShared > > maybeShared = None;
  Option< own< TestUnique > > maybeUnique = None;

  STATIC_ASSERT_TEST(is_same< int, decltype(maybeInt.unwrapOr(42)) >);
  STATIC_ASSERT_TEST(is_same< own< TestShared >, decltype(maybeShared.unwrapOr(make< TestShared >())) >);
  STATIC_ASSERT_TEST(is_same< own< TestUnique >, decltype(maybeUnique.unwrapOr(make< TestUnique >())) >);

  ASSERT_TRUE(maybeShared.isNone());
  auto shared = maybeShared.unwrapOr(make< TestShared >());
  ASSERT_EQ(42, shared->value);
  ASSERT_TRUE(maybeShared.isNone());
  ASSERT_FALSE(isShared(shared)); // this is the only reference

  ASSERT_TRUE(maybeUnique.isNone());
  auto unique = maybeUnique.unwrapOr(make< TestUnique >());
  ASSERT_EQ(42, unique->value);
  ASSERT_TRUE(maybeUnique.isNone());
}

TEST(Simple, unwrapOrFunc) {
  Option< int > maybeInt = None;
  Option< own< TestShared > > maybeShared = None;
  Option< own< TestUnique > > maybeUnique = None;

  ASSERT_TRUE(maybeShared.isNone());
  auto shared = maybeShared.unwrapOr([] { return make< TestShared >(); });
  STATIC_ASSERT_TEST(is_same< own< TestShared >, decltype(shared) >);
  ASSERT_EQ(42, shared->value);
  ASSERT_TRUE(maybeShared.isNone());
  ASSERT_FALSE(isShared(shared)); // this is the only reference

  ASSERT_TRUE(maybeUnique.isNone());
  auto unique = maybeUnique.unwrapOr([] { return make< TestUnique >(); });
  STATIC_ASSERT_TEST(is_same< own< TestUnique >, decltype(unique) >);
  ASSERT_EQ(42, unique->value);
  ASSERT_TRUE(maybeUnique.isNone());
}

TEST(Simple, mapValue) {
  Option< int > surelyInt = Some(42);
  Option< own< TestShared > > surelyShared = Some(make< TestShared >());
  Option< own< TestUnique > > surelyUnique = Some(make< TestUnique >());

  auto number = surelyInt.map([](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(number) >);
  ASSERT_EQ(84, number.unwrap());
  ASSERT_TRUE(surelyInt.isNone());

  auto shared = surelyShared.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(shared) >);
  ASSERT_EQ(84, shared.unwrap());
  ASSERT_TRUE(surelyShared.isNone());

  auto unique = surelyUnique.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(unique) >);
  ASSERT_EQ(84, unique.unwrap());
  ASSERT_TRUE(surelyUnique.isNone());
}

TEST(Simple, mapNone) {
  Option< int > notInt = None;
  Option< own< TestShared > > notShared = None;
  Option< own< TestUnique > > notUnique = None;

  auto number = notInt.map([](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(number) >);
  ASSERT_TRUE(number.isNone());
  ASSERT_TRUE(notInt.isNone());

  auto shared = notShared.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(shared) >);
  ASSERT_TRUE(shared.isNone());
  ASSERT_TRUE(notShared.isNone());

  auto unique = notUnique.map([](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(unique) >);
  ASSERT_TRUE(unique.isNone());
  ASSERT_TRUE(notUnique.isNone());
}

TEST(Simple, mapOrValue) {
  Option< int > notInt = None;
  Option< own< TestShared > > notShared = None;
  Option< own< TestUnique > > notUnique = None;

  auto number = notInt.mapOr(21, [](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(number) >);
  ASSERT_EQ(21, number.unwrap());
  ASSERT_TRUE(notInt.isNone());

  auto shared = notShared.mapOr(21, [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(shared) >);
  ASSERT_EQ(21, shared.unwrap());
  ASSERT_TRUE(notShared.isNone());

  auto unique = notUnique.mapOr(21, [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(unique) >);
  ASSERT_EQ(21, unique.unwrap());
  ASSERT_TRUE(notUnique.isNone());
}

TEST(Simple, mapOrFunc) {
  Option< int > notInt = None;
  Option< own< TestShared > > notShared = None;
  Option< own< TestUnique > > notUnique = None;

  auto number = notInt.mapOr([] { return 1; }, [](auto &&s) { return s * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(number) >);
  ASSERT_EQ(1, number.unwrap());
  ASSERT_TRUE(notInt.isNone());

  auto shared = notShared.mapOr([] { return 1; }, [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(shared) >);
  ASSERT_EQ(1, shared.unwrap());
  ASSERT_TRUE(notShared.isNone());

  auto unique = notUnique.mapOr([] { return 1; }, [](auto &&s) { return s->value * 2; });
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(unique) >);
  ASSERT_EQ(1, unique.unwrap());
  ASSERT_TRUE(notUnique.isNone());
}

TEST(Simple, andPositive) {
  Option< int > surelyInt = Some(42);
  Option< own< TestShared > > surelyShared = Some(make< TestShared >());
  Option< own< TestUnique > > surelyUnique = Some(make< TestUnique >());

  auto number = surelyInt.and_(Some(21));
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(number) >);
  ASSERT_EQ(21, number.unwrap());
  ASSERT_TRUE(surelyInt.isSome());

  auto shared = surelyShared.and_(Some(21));
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(shared) >);
  ASSERT_EQ(21, shared.unwrap());
  ASSERT_TRUE(surelyShared.isSome());

  auto unique = surelyUnique.and_(Some(21));
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(unique) >);
  ASSERT_EQ(21, unique.unwrap());
  ASSERT_TRUE(surelyUnique.isSome());
}

TEST(Simple, andNegative) {
  Option< int > surelyInt = None;
  Option< own< TestShared > > surelyShared = None;
  Option< own< TestUnique > > surelyUnique = None;

  auto number = surelyInt.and_(Some(21));
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(number) >);
  ASSERT_TRUE(number.isNone());

  auto shared = surelyShared.and_(Some(21));
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(shared) >);
  ASSERT_TRUE(shared.isNone());

  auto unique = surelyUnique.and_(Some(21));
  STATIC_ASSERT_TEST(is_same< Option< int >, decltype(unique) >);
  ASSERT_TRUE(unique.isNone());
}

TEST(Simple, comparison) {
  auto surelyInt = Some(42);
  Option<int> notInt = None;

  ASSERT_FALSE(notInt == surelyInt);
  ASSERT_TRUE(notInt == None);
  ASSERT_TRUE(surelyInt == Some(42));
  ASSERT_FALSE(surelyInt != Some(42));
  ASSERT_FALSE(surelyInt == Some(21));
  ASSERT_TRUE(surelyInt != Some(21));
  ASSERT_FALSE(surelyInt == None);
  ASSERT_TRUE(surelyInt != None);
  ASSERT_TRUE(surelyInt > Some(21));
  ASSERT_FALSE(surelyInt < Some(21));
}
