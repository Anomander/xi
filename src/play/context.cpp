#include <array>
#include <functional>
#include <iostream>

#include <boost/context/all.hpp>

using namespace boost::context;

class Coroutine {
public:
  Coroutine()
      : my_context(boost::context::make_fcontext(
            stack.data() + stack.size(), stack.size(), Coroutine::dispatch)) {
  }
  virtual ~Coroutine() {
  }

  void operator()() {
    boost::context::jump_fcontext(
        &yield_context, my_context, reinterpret_cast< intptr_t >(this));
  }

protected:
  void yield() {
    boost::context::jump_fcontext(my_context, &yield_context, 0);
  }

  virtual void call() = 0;

private:
  static void dispatch(intptr_t coroutine_ptr) {
    Coroutine *coroutine = reinterpret_cast< Coroutine * >(coroutine_ptr);
    coroutine->call();
    while (true)
      coroutine->yield();
  }

private:
  std::array< intptr_t, 64 * 1024 > stack;
  boost::context::fcontext_t *my_context;
  boost::context::fcontext_t yield_context;
};

struct A : public Coroutine {
  void call() {
    std::cerr << "A went to the store one day.\n";
    yield();
    std::cerr << "A was looking for groceries.\n";
    yield();
    std::cerr << "A finally found what she was looking for.\n";
  }
};

struct B : public Coroutine {
  void call() {
    std::cerr << "B went to the store one day.\n";
    yield();
    std::cerr << "B was looking for replacement tires.\n";
    yield();
    std::cerr << "B didn't find anything at all.\n";
    yield();
    std::cerr << "B went to another store.\n";
    yield();
    std::cerr << "B got the tires installed there.\n";
  }
};

struct C : public Coroutine {
  void call() {
    std::cerr << "C went to the store one day.\n";
    yield();
    std::cerr << "C was looking for USB drives.\n";
    yield();
    std::cerr << "C found several with competitive pricing.\n";
    yield();
    std::cerr << "C couldn't decide which to buy, so gave up.\n";
  }
};

int
old_main() {
  std::cerr << "So, this is what happened.\n";
  A a;
  B b;
  C c;
  for (size_t i = 0; i < 10; ++i) {
    a();
    b();
    c();
  }
  std::cerr << "Then it all was done.\n";
  return 0;
}

fcontext_t mainc, *fc;

void
f1(intptr_t arg) {
  std::cout << "1: " << arg << std::endl;
  jump_fcontext(fc, &mainc, 0);
  std::cout << "2: " << arg << std::endl;
  jump_fcontext(fc, &mainc, 0);
}

int
main() {
  std::array< intptr_t, 64 * 1024 > stack;
  fc = (make_fcontext(stack.data() + stack.size(), stack.size(), f1));
  jump_fcontext(&mainc, fc, 42);
  jump_fcontext(&mainc, fc, 43);
}
