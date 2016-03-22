#pragma once

#include "xi/core/shard.h"
#include "xi/ext/configure.h"

namespace xi {
namespace async {

  template < class T >
  class async {
    mut< core::shard > _shard;

  protected:
    virtual ~async() = default;

  public:
    virtual void attach_shard(mut< core::shard > e) {
      _shard = e;
    }

    template < class F >
    void defer(F &&f) {
      if (!is_valid(_shard)) {
        throw std::logic_error("Invalid async object state.");
      }
      _shard->post(forward< F >(f));
    }

  private:
    friend T;
    mut< core::shard > shard() {
      assert(is_valid(_shard));
      return _shard;
    }
  };
}
}
