#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/detail/Links.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class M >
      struct ReadLinkImpl : virtual ReadLink< M > {
        mut< ReadLink< M > > nextRead(M* = nullptr) { return _nextRead; }

        void addLinkIfNull(mut< GenericFilterContext > cx) {
          auto readM = dynamic_cast< mut< ReadLink< M > > >(cx);
          if (readM && nullptr == _nextRead) {
            _nextRead = readM;
          }
        }

      private:
        mut< ReadLink< M > > _nextRead = nullptr;
      };

      template < class M >
      struct WriteLinkImpl : virtual WriteLink< M > {
        mut< WriteLink< M > > nextWrite(M* = nullptr) { return _nextWrite; }

        void addLinkIfNull(mut< GenericFilterContext > cx) {
          auto writeM = dynamic_cast< mut< WriteLink< M > > >(cx);
          if (writeM && nullptr == _nextWrite) {
            _nextWrite = writeM;
          }
        }

      private:
        mut< WriteLink< M > > _nextWrite = nullptr;
      };
    }
  }
}
}
