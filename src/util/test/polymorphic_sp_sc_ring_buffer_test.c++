#include "xi/util/polymorphic_sp_sc_ring_buffer.h"

#include <gtest/gtest.h>

using namespace xi;

template < class T >
using ring_buffer = xi::polymorphic_sp_sc_ring_buffer< T >;

TEST(simple, push_pop) {
  ring_buffer< int > rb(1000);
  ASSERT_TRUE(rb.push(1));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(simple, push_push_pop_pop) {
  ring_buffer< int > rb(1000);
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

TEST(simple, not_enough_space_for_push) {
  ring_buffer< int > rb(1);
  ASSERT_FALSE(rb.push(1));
  ASSERT_EQ(nullptr, rb.next());
}

TEST(simple, barely_enough_space_for_push) {
  ring_buffer< int > rb(17);
  ASSERT_TRUE(rb.push(1));
  ASSERT_FALSE(rb.push(2));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(simple, not_enough_space_to_push) {
  ring_buffer< int > rb(24);
  ASSERT_TRUE(rb.push(1));
  ASSERT_FALSE(rb.push(2));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(simple, wrapping_around_because_not_much_size_left) {
  ring_buffer< int > rb(51);
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

TEST(simple, wrapping_around_with_a_marker) {
  ring_buffer< int > rb(60);
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

TEST(simple, wrapping_around_multiple_times) {
  ring_buffer< int > rb(60);
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

TEST(reserve, non_force_push_respects_reserve) {
  ring_buffer< int > rb(60, 60);
  ASSERT_FALSE(rb.push(1));
  ASSERT_EQ(nullptr, rb.next());
}

TEST(reserve, force_push_ignores_reserve) {
  ring_buffer< int > rb(60, 60);
  ASSERT_TRUE(rb.push_forced(1));
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, *rb.next());
  rb.pop();
}

struct base {
  virtual ~base()   = default;
  virtual int run() = 0;
};

struct D1 : public base {
  int run() override {
    return 1;
  }

private:
  char pad[1];
};

struct D2 : public D1 {
  int run() override {
    return D1::run() + 2;
  }

private:
  char pad[5];
};

struct D3 : public D2 {
  int run() override {
    return D2::run() + 3;
  }

private:
  char pad[15];
};

TEST(polymorphic, D1) {
  ring_buffer< base > rb(1000);
  rb.push(D1{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(1, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(polymorphic, D2) {
  ring_buffer< base > rb(1000);
  rb.push(D2{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(3, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(polymorphic, D3) {
  ring_buffer< base > rb(1000);
  rb.push(D3{});
  ASSERT_NE(nullptr, rb.next());
  ASSERT_EQ(6, rb.next()->run());
  rb.pop();
  ASSERT_EQ(nullptr, rb.next());
}

TEST(polymorphic, D1D2D3) {
  ring_buffer< base > rb(1000);
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

struct ctor_throws : public base {
  ctor_throws() = default;
  ctor_throws(const ctor_throws &) {
    throw std::exception();
  }
  ctor_throws(ctor_throws &&) {
    throw std::exception();
  }
  int run() override {
    return 0;
  }
};

TEST(polymorphic, constructor_exception) {
  ring_buffer< base > rb(1000);
  ctor_throws bad;
  ASSERT_THROW(rb.push(bad), std::exception);
  ASSERT_EQ(nullptr, rb.next());
}

struct copy_move_distinguisher : public base {
  static int COPY;
  static int MOVE;
  copy_move_distinguisher() = default;
  copy_move_distinguisher(const copy_move_distinguisher &) {
    COPY++;
  }
  copy_move_distinguisher(copy_move_distinguisher &&) {
    MOVE++;
  }
  int run() override {
    return 0;
  }
};
int copy_move_distinguisher::COPY = 0;
int copy_move_distinguisher::MOVE = 0;

TEST(polymorphic, prefers_move_when_able) {
  ring_buffer< base > rb(1000);
  copy_move_distinguisher obj;
  ASSERT_TRUE(rb.push(obj));
  ASSERT_EQ(0, copy_move_distinguisher::MOVE);
  ASSERT_EQ(1, copy_move_distinguisher::COPY);
  ASSERT_TRUE(rb.push(move(obj)));
  ASSERT_EQ(1, copy_move_distinguisher::MOVE);
  ASSERT_EQ(1, copy_move_distinguisher::COPY);
}

struct destrutor_notifier : public base {
  static int DESTROYED;
  ~destrutor_notifier() {
    DESTROYED++;
  }
  int run() override {
    return 0;
  }
};
int destrutor_notifier::DESTROYED = 0;

TEST(polymorphic, calls_destructor_on_pop) {
  ring_buffer< base > rb(1000);
  ASSERT_TRUE(rb.push(destrutor_notifier{}));
  ASSERT_EQ(1, destrutor_notifier::DESTROYED); // temporary destroyed
  ASSERT_TRUE(rb.push(destrutor_notifier{}));
  ASSERT_EQ(2, destrutor_notifier::DESTROYED); // temporary destroyed
  rb.pop();
  ASSERT_EQ(3, destrutor_notifier::DESTROYED);
  rb.pop();
  ASSERT_EQ(4, destrutor_notifier::DESTROYED);
}

// static constexpr uint16_t ki_b = 1024;
// static constexpr uint32_t mi_b = 1024 * ki_b;
// static constexpr uint64_t gi_b = 1024 * mi_b;

// TEST(parallel, load) {
//   ring_buffer< base > rb(100 * mi_b);
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
//         std::cerr << "Failed to push. retrying." << std::endl;
//       }
//     }
//   } };
//   thread consumer{ [&] {
//     size_t counts[3] = { 0, 0, 0 };
//     size_t c = 0;
//     base* n = nullptr;
//     while (true) {
//       if (!(n = rb.next())) {
//         std::cerr << "Failed to read. retrying." << std::endl;
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
//         std::cout << " [" << c << "] running: {" << counts[0] << " " <<
//         counts[1] << " " << counts[2] << "}"
//                   << std::endl;
//       }
//       rb.pop();
//     }
//   } };
//   consumer.join();
// }
