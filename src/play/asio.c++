#include <thread>
#include <boost/asio.hpp>

using namespace std;

enum { kNumThreads = 2 };

vector< boost::asio::io_service > S(kNumThreads);

struct ForkBombAsio {
  ForkBombAsio(size_t i = 0) : _i(i) {}
  void operator()() {
    // if (_i % 10000 == 0) {
    //   std::cout << pthread_self() << " : " << _i << std::endl;
    // }
    for (int i = 0; i < kNumThreads; ++i) {
      S[i].post(ForkBombAsio(_i + 1));
    }
  };

  size_t _i;
};

int main(int argc, char* argv[]) {
  for (int i = 0; i < kNumThreads; ++i) {
    S[i].post(ForkBombAsio(0));
  }

  vector< thread > _t;
  for (int i = 0; i < kNumThreads; ++i) {
    _t.emplace_back([i = i] {
      cpu_set_t cs;
      CPU_ZERO(&cs);
      CPU_SET(i, &cs);
      auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
      assert(r == 0);
      S[i].run();
    });
  }
  _t[0].join();
  return 0;
}
