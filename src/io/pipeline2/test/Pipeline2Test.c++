#include "xi/io/pipeline2/Pipeline.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::pipe2;

class ReadOnlyIntFilter : public Filter<ReadOnly<int>> {
  void read(mut<Context> cx, int i) override {
    cx->forwardRead(I += ++i);
  }
 public:
  int I = 0;
};

class WriteOnlyIntFilter : public Filter<WriteOnly<int>> {
  void write(mut<Context> cx, int i) override {
    cx->forwardWrite(I += ++i);
  }
public:
  int I = 0;
};

class IntFilter : public Filter<int> {
  void read(mut<Context> cx, int i) override {
    cx->forwardRead(I += ++i);
  }
  void write(mut<Context> cx, int i) override {
    cx->forwardWrite(I += ++i);
  }
public:
  int I = 0;
};

class StringFilter : public Filter<string> {
  void read(mut<Context> cx, string s) override {
    cx->forwardRead(S += s);
  }
  void write(mut<Context> cx, string s) override {
    cx->forwardWrite(S += s);
  }
public:
  string S;
};

TEST(Simple, readsArriveInCorrectOrder){
  auto pipe = make<Pipe<int>>();
  auto f1 = make<IntFilter>();
  auto f2 = make<IntFilter>();
  pipe->pushBack(share(f1));
  pipe->pushBack(share(f2));

  pipe->read(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(3, f2->I);
}

TEST(Simple, writesArriveInCorrectOrder){
  auto pipe = make<Pipe<int>>();
  auto f1 = make<IntFilter>();
  auto f2 = make<IntFilter>();
  pipe->pushBack(share(f1));
  pipe->pushBack(share(f2));

  pipe->write(1);
  ASSERT_EQ(3, f1->I);
  ASSERT_EQ(2, f2->I);
}

TEST(Simple, sharedFilter){
  auto pipe1 = make<Pipe<int>>();
  auto pipe2 = make<Pipe<int>>();
  auto f = make<IntFilter>();

  pipe1->pushBack(share(f));
  pipe2->pushBack(share(f));

  pipe1->write(1);
  pipe2->read(1);
  ASSERT_EQ(4, f->I);
}

TEST(Simple, writeOnlyHandlerIsSkippedOnReads) {
  auto pipe = make<Pipe<int>>();
  auto f1 = make<WriteOnlyIntFilter>();
  auto f2 = make<IntFilter>();
  pipe->pushBack(share(f2));
  pipe->pushBack(share(f1));

  pipe->read(1);
  ASSERT_EQ(0, f1->I);
  ASSERT_EQ(2, f2->I);

  pipe->write(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(5, f2->I);
}

TEST(Simple, readOnlyHandlerIsSkippedOnWrites) {
  auto pipe = make<Pipe<int>>();
  auto f1 = make<ReadOnlyIntFilter>();
  auto f2 = make<IntFilter>();
  pipe->pushBack(share(f1));
  pipe->pushBack(share(f2));

  pipe->read(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(3, f2->I);

  pipe->write(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(5, f2->I);
}

TEST(Simple, readOnlyPipeCanStillPassWritesBetweenFilters) {
  class Filter1 : public Filter<int> {
    void read(mut<Context> cx, int i) override {
      cx->forwardRead(I += ++i);
    }
    void write(mut<Context> cx, int i) override {
      I += ++i;
    }
  public:
    int I = 0;
  };
  class Filter2 : public Filter<int> {
    void read(mut<Context> cx, int i) override {
      cx->forwardWrite(I += ++i);
    }
  public:
    int I = 0;
  };
  auto pipe = make<Pipe<ReadOnly<int>>>();
  auto f1 = make<Filter1>();
  auto f2 = make<Filter2>();
  pipe->pushBack(share(f1));
  pipe->pushBack(share(f2));

  pipe->read(1);
  ASSERT_EQ(2+4, f1->I);
  ASSERT_EQ(3, f2->I);
}

TEST(Simple, writeOnlyPipeCanStillPassReadsBetweenFilters) {
  class Filter1 : public Filter<int> {
    void write(mut<Context> cx, int i) override {
      cx->forwardWrite(I += ++i);
    }
    void read(mut<Context> cx, int i) override {
      I += ++i;
    }
  public:
    int I = 0;
  };
  class Filter2 : public Filter<int> {
    void write(mut<Context> cx, int i) override {
      cx->forwardRead(I += ++i);
    }
  public:
    int I = 0;
  };
  auto pipe = make<Pipe<WriteOnly<int>>>();
  auto f1 = make<Filter1>();
  auto f2 = make<Filter2>();
  pipe->pushBack(share(f2));
  pipe->pushBack(share(f1));

  pipe->write(1);
  ASSERT_EQ(2+4, f1->I);
  ASSERT_EQ(3, f2->I);
}

TEST(Simple, skipLevelMessagePassing) {
  auto pipe = make<Pipe<int>>();
  auto f1 = make<IntFilter>();
  auto f2 = make<IntFilter>();
  pipe->pushBack(share(f1));
  pipe->pushBack(make<StringFilter>());
  pipe->pushBack(share(f2));

  pipe->read(1);
  ASSERT_EQ(2, f1->I);
  ASSERT_EQ(3, f2->I);

  pipe->write(1);
  ASSERT_EQ(8, f1->I);
  ASSERT_EQ(5, f2->I);
}
