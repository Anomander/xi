#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  class shard;
}
namespace core {

  template < class T >
  class async {
    mut< core::shard > _shard;

  protected:
    virtual ~async() = default;

  public:
    virtual void attach_shard(mut< core::shard > e) {
      _shard = e;
    }

    mut< core::shard > shard() {
      assert(is_valid(_shard));
      return _shard;
    }
  };
}
}
