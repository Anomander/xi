#include "xi/io/pipes/all.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::pipes;

class read_only_int_filter : public filter< read_only< int > > {
  void read(mut< context > cx, int i) override { cx->forward_read(I += ++i); }

public:
  int I = 0;
};

class write_only_int_filter : public filter< write_only< int > > {
  void write(mut< context > cx, int i) override { cx->forward_write(I += ++i); }

public:
  int I = 0;
};

class int_filter : public filter< int > {
  void read(mut< context > cx, int i) override { cx->forward_read(I += ++i); }
  void write(mut< context > cx, int i) override { cx->forward_write(I += ++i); }

public:
  int I = 0;
};

class string_filter : public filter< string > {
  void read(mut< context > cx, string s) override { cx->forward_read(S += s); }
  void write(mut< context > cx, string s) override {
    cx->forward_write(S += s);
  }

public:
  string S;
};

TEST(simple, reads_arrive_in_correct_order) {
  auto p = make< pipes::pipe< int > >();
  auto f1 = make< int_filter >();
  auto f2 = make< int_filter >();
  p->push_back(share(f1));
  p->push_back(share(f2));

  p->read(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(3, f2->I);
}

TEST(simple, writes_arrive_in_correct_order) {
  auto p = make< pipes::pipe< int > >();
  auto f1 = make< int_filter >();
  auto f2 = make< int_filter >();
  p->push_back(share(f1));
  p->push_back(share(f2));

  p->write(1);
  ASSERT_EQ(3, f1->I);
  ASSERT_EQ(2, f2->I);
}

TEST(simple, shared_filter) {
  auto p1 = make< pipes::pipe< int > >();
  auto p2 = make< pipes::pipe< int > >();
  auto f = make< int_filter >();

  p1->push_back(share(f));
  p2->push_back(share(f));

  p1->write(1);
  p2->read(1);
  ASSERT_EQ(4, f->I);
}

TEST(simple, write_only_handler_is_skipped_on_reads) {
  auto p = make< pipes::pipe< int > >();
  auto f1 = make< write_only_int_filter >();
  auto f2 = make< int_filter >();
  p->push_back(share(f2));
  p->push_back(share(f1));

  p->read(1);
  ASSERT_EQ(0, f1->I);
  ASSERT_EQ(2, f2->I);

  p->write(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(5, f2->I);
}

TEST(simple, read_only_handler_is_skipped_on_writes) {
  auto p = make< pipes::pipe< int > >();
  auto f1 = make< read_only_int_filter >();
  auto f2 = make< int_filter >();
  p->push_back(share(f1));
  p->push_back(share(f2));

  p->read(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(3, f2->I);

  p->write(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(5, f2->I);
}

TEST(simple, read_only_pipe_can_still_pass_writes_between_filters) {
  class filter1 : public filter< int > {
    void read(mut< context > cx, int i) override { cx->forward_read(I += ++i); }
    void write(mut< context > cx, int i) override { I += ++i; }

  public:
    int I = 0;
  };
  class filter2 : public filter< int > {
    void read(mut< context > cx, int i) override {
      cx->forward_write(I += ++i);
    }

  public:
    int I = 0;
  };
  auto p = make< pipes::pipe< read_only< int > > >();
  auto f1 = make< filter1 >();
  auto f2 = make< filter2 >();
  p->push_back(share(f1));
  p->push_back(share(f2));

  p->read(1);
  ASSERT_EQ(2 + 4, f1->I);
  ASSERT_EQ(3, f2->I);
}

TEST(simple, write_only_pipe_can_still_pass_reads_between_filters) {
  class filter1 : public filter< int > {
    void write(mut< context > cx, int i) override {
      cx->forward_write(I += ++i);
    }
    void read(mut< context > cx, int i) override { I += ++i; }

  public:
    int I = 0;
  };
  class filter2 : public filter< int > {
    void write(mut< context > cx, int i) override {
      cx->forward_read(I += ++i);
    }

  public:
    int I = 0;
  };
  auto p = make< pipes::pipe< write_only< int > > >();
  auto f1 = make< filter1 >();
  auto f2 = make< filter2 >();
  p->push_back(share(f2));
  p->push_back(share(f1));

  p->write(1);
  ASSERT_EQ(2 + 4, f1->I);
  ASSERT_EQ(3, f2->I);
}

TEST(simple, skip_level_message_passing) {
  auto p = make< pipes::pipe< int > >();
  auto f1 = make< int_filter >();
  auto f2 = make< int_filter >();
  p->push_back(share(f1));
  p->push_back(make< string_filter >());
  p->push_back(share(f2));

  p->read(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(3, f2->I);

  p->write(1);
  ASSERT_EQ(8, f1->I);
  ASSERT_EQ(5, f2->I);
}
