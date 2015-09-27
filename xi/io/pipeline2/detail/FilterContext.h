#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/Modifiers.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class... Messages >
      struct FilterContext;

      template < class M0, class... Messages >
      struct FilterContext< M0, Messages... > : virtual FilterContext< Messages... > {
        using FilterContext< Messages... >::forwardRead;
        using FilterContext< Messages... >::forwardWrite;

        virtual void forwardRead(M0 m) = 0;
        virtual void forwardWrite(M0 m) = 0;
      };

      template < class M0, class... Messages >
      struct FilterContext< ReadOnly< M0 >, Messages... > : virtual FilterContext< Messages... > {
        using FilterContext< Messages... >::forwardRead;
        using FilterContext< Messages... >::forwardWrite;

        virtual void forwardRead(M0 m) = 0;
      };

      template < class M0, class... Messages >
      struct FilterContext< WriteOnly< M0 >, Messages... > : virtual FilterContext< Messages... > {
        using FilterContext< Messages... >::forwardRead;
        using FilterContext< Messages... >::forwardWrite;

        virtual void forwardWrite(M0 m) = 0;
      };

      template <>
      struct FilterContext<> {
        virtual ~FilterContext() = default;
        void forwardRead() = delete;
        void forwardWrite() = delete;
      };
    }
  }
}
}
