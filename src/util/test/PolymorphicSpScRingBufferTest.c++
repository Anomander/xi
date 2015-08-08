#include "util/PolymorphicSpScRingBuffer.h"

#include <gtest/gtest.h>

using namespace xi;

template < class T > using RingBuffer = xi::PolymorphicSpScRingBuffer< T >;

TEST(Simple, PushPop) {
  RingBuffer< int > rb(1000);
  ASSERT_TRUE(rb.push(1));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, PushPushPopPop) {
  RingBuffer< int > rb(1000);
  ASSERT_TRUE(rb.push(1));
  ASSERT_TRUE(rb.push(2));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(2, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, NotEnoughSpaceForPush) {
  RingBuffer< int > rb(1);
  ASSERT_FALSE(rb.push(1));
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, BarelyEnoughSpaceForPush) {
  RingBuffer< int > rb(17);
  ASSERT_TRUE(rb.push(1));
  ASSERT_FALSE(rb.push(2));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, NotEnoughSpaceToPush) {
  RingBuffer< int > rb(24);
  ASSERT_TRUE(rb.push(1));
  ASSERT_FALSE(rb.push(2));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, WrappingAroundBecauseNotMuchSizeLeft) {
  RingBuffer< int > rb(51);
  ASSERT_TRUE(rb.push(1));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_TRUE(rb.push(2));
  ASSERT_TRUE(rb.push(3));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(2, *rb.next());
  rb.pop();
  ASSERT_TRUE(rb.push(4)); // wraps around
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(3, *rb.next());
  rb.pop();
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(4, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, WrappingAroundWithAMarker) {
  RingBuffer< int > rb(60);
  ASSERT_TRUE(rb.push(1));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_TRUE(rb.push(2));
  ASSERT_TRUE(rb.push(3));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(2, *rb.next());
  rb.pop();
  ASSERT_TRUE(rb.push(4)); // wraps around
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(3, *rb.next());
  rb.pop();
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(4, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Simple, WrappingAroundMultipleTimes) {
  RingBuffer< int > rb(60);
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(rb.push(1));
    ASSERT_NE(nullptr, rb.next());
    ASSERT_EQ(1, *rb.next());
    rb.pop();
    ASSERT_TRUE(rb.push(2));
    ASSERT_TRUE(rb.push(3));
    ASSERT_NE(nullptr, rb.next());
    ASSERT_EQ(2, *rb.next());
    rb.pop();
    ASSERT_TRUE(rb.push(4)); // wraps around
    ASSERT_NE(nullptr, rb.next());
    ASSERT_EQ(3, *rb.next());
    rb.pop();
    ASSERT_NE(nullptr, rb.next());
    ASSERT_EQ(4, *rb.next());
    rb.pop();
    ASSERT_EQ(nullptr, rb.next());
  }
}

struct Base {
  virtual ~Base() = default;
  virtual int run() = 0;
};

struct D1 : public Base {
  int run() override { return 1; }

private:
  char pad[1];
};

struct D2 : public D1 {
  int run() override { return D1::run() + 2; }

private:
  char pad[5];
};

struct D3 : public D2 {
  int run() override { return D2::run() + 3; }

private:
  char pad[15];
};

TEST(Polymorphic, D1) {
  RingBuffer< Base > rb(1000);
  rb.push(D1{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Polymorphic, D2) {
  RingBuffer< Base > rb(1000);
  rb.push(D2{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(3, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Polymorphic, D3) {
  RingBuffer< Base > rb(1000);
  rb.push(D3{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(6, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(Polymorphic, D1D2D3) {
  RingBuffer< Base > rb(1000);
  rb.push(D1{});
  rb.push(D2{});
  rb.push(D3{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, rb.next()->run());
  rb.pop();
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(3, rb.next()->run());
  rb.pop();
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(6, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

struct CtorThrows : public Base {
  CtorThrows() = default;
  CtorThrows(const CtorThrows &) { throw std::exception(); }
  CtorThrows(CtorThrows &&) { throw std::exception(); }
  int run() override { return 0; }
};

TEST(Polymorphic, ConstructorException) {
  RingBuffer< Base > rb(1000);
  CtorThrows bad;
  ASSERT_THROW(rb.push(bad), std::exception);
  ASSERT_EQ(nullptr, rb.next());
}

struct CopyMoveDistinguisher : public Base {
  static int COPY;
  static int MOVE;
  CopyMoveDistinguisher() = default;
  CopyMoveDistinguisher(const CopyMoveDistinguisher &) { COPY++; }
  CopyMoveDistinguisher(CopyMoveDistinguisher &&) { MOVE++; }
  int run() override { return 0; }
};
int CopyMoveDistinguisher::COPY = 0;
int CopyMoveDistinguisher::MOVE = 0;

TEST(Polymorphic, PrefersMoveWhenAble) {
  RingBuffer< Base > rb(1000);
  CopyMoveDistinguisher obj;
  ASSERT_TRUE(rb.push(obj));
  ASSERT_EQ(0, CopyMoveDistinguisher::MOVE);
  ASSERT_EQ(1, CopyMoveDistinguisher::COPY);
  ASSERT_TRUE(rb.push(move(obj)));
  ASSERT_EQ(1, CopyMoveDistinguisher::MOVE);
  ASSERT_EQ(1, CopyMoveDistinguisher::COPY);
}

struct DestrutorNotifier : public Base {
  static int DESTROYED;
  ~DestrutorNotifier() { DESTROYED++; }
  int run() override { return 0; }
};
int DestrutorNotifier::DESTROYED = 0; 

TEST(Polymorphic, CallsDestructorOnPop) {
  RingBuffer< Base > rb(1000);
  ASSERT_TRUE(rb.push(DestrutorNotifier{}));
  ASSERT_EQ(1, DestrutorNotifier::DESTROYED); // temporary destroyed
  ASSERT_TRUE(rb.push(DestrutorNotifier{}));
  ASSERT_EQ(2, DestrutorNotifier::DESTROYED); // temporary destroyed
  rb.pop();
  ASSERT_EQ(3, DestrutorNotifier::DESTROYED);
  rb.pop();
  ASSERT_EQ(4, DestrutorNotifier::DESTROYED);
}

// static constexpr uint16_t KiB = 1024;
// static constexpr uint32_t MiB = 1024 * KiB;
// static constexpr uint64_t GiB = 1024 * MiB;

// TEST(Parallel, Load) {
//   RingBuffer< Base > rb(100 * MiB);
//   thread producer{ [&] {
//     size_t c = 0;
//     while (true) {
//       bool result = false;
//       switch (c++ % 3) {
//         case 0:
//           result = rb.push(D1{});
//           break;
//         case 1:
//           result = rb.push(D2{});
//           break;
//         case 2:
//           result = rb.push(D3{});
//           break;
//       }
//       if (!result) {
//         std::cerr << "Failed to push. Retrying." << std::endl;
//       }
//     }
//   } };
//   thread consumer{ [&] {
//     size_t counts[3] = { 0, 0, 0 };
//     size_t c = 0;
//     Base* n = nullptr;
//     while (true) {
//       if (!(n = rb.next())) {
//         std::cerr << "Failed to read. Retrying." << std::endl;
//         continue;
//       }
//       switch (n->run()) {
//         case 1:
//           ++counts[0];
//           break;
//         case 3:
//           ++counts[1];
//           break;
//         case 6:
//           ++counts[2];
//           break;
//       }
//       if ((++c % 10000) == 0) {
//         std::cout << " [" << c << "] Running: {" << counts[0] << " " << counts[1] << " " << counts[2] << "}"
//                   << std::endl;
//       }
//       rb.pop();
//     }
//   } };
//   consumer.join();
// }
