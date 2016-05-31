#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  namespace v2 {

    /// TODO: Needs compatibitily check
    class execution_budget {
      usize _allocated;
      usize _jitter;
      usize _start_tsc;
      usize _curr_tsc;

    public:
      execution_budget(usize a, usize jitter = 0)
          : _allocated(a)
          , _jitter(jitter)
          , _start_tsc(_rdtscl())
          , _curr_tsc(_start_tsc) {
      }

      bool adjust_spent() {
        _curr_tsc = _rdtscl();
        return !is_expended();
      }

      usize remainder() const {
        return ((_curr_tsc + _jitter) - _start_tsc) - _allocated;
      }

      bool is_expended() const {
        return (_curr_tsc + _jitter) - _start_tsc >= _allocated;
      }

      execution_budget sub_budget(usize a) {
        return execution_budget(
            min(remainder(), a), _jitter, _start_tsc, _rdtscl());
      }

    private:
      execution_budget(usize a, usize jitter, usize start, usize curr)
          : _allocated(a), _jitter(jitter), _start_tsc(start), _curr_tsc(curr) {
      }

      usize _rdtscl() {
        unsigned int lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return ((usize)lo) | (((usize)hi) << 32);
      }
    };
  }
}
}
