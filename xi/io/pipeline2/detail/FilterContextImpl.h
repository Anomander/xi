#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/Modifiers.h"
#include "xi/io/pipeline2/detail/LinksImpl.h"
#include "xi/io/pipeline2/detail/FilterContext.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class C, class M0 >
      struct FilterContextImpl : WriteLinkImpl< M0 >, ReadLinkImpl< M0 >, virtual C {

        void forwardRead(M0 m) final override {
          auto nextRead = static_cast< ReadLinkImpl< M0 >* >(this)->nextRead();
          if (nextRead) {
            nextRead->read(move(m));
          }
        }
        void forwardWrite(M0 m) final override {
          auto nextWrite = static_cast< WriteLinkImpl< M0 >* >(this)->nextWrite();
          if (nextWrite) {
            nextWrite->write(move(m));
          }
        }
      };

      template < class C, class M0 >
      struct FilterContextImpl< C, ReadOnly< M0 > > : ReadLinkImpl< M0 >, virtual C {

        void forwardRead(M0 m) final override {
          auto nextRead = static_cast< ReadLinkImpl< M0 >* >(this)->nextRead();
          if (nextRead) {
            nextRead->read(move(m));
          }
        }
      };

      template < class C, class M0 >
      struct FilterContextImpl< C, WriteOnly< M0 > > : WriteLinkImpl< M0 >, virtual C {

        void forwardWrite(M0 m) final override {
          auto nextWrite = static_cast< WriteLinkImpl< M0 >* >(this)->nextWrite();
          if (nextWrite) {
            nextWrite->write(move(m));
          }
        }
      };
    }
  }
}
}
