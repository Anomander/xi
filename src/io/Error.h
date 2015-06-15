#pragma once

#include "ext/Error.h"

namespace xi{
  namespace io {
    namespace error {

    enum Enumeration {
      kEof,
      kUnableToReadMessageHeader,
    };

    XI_DECLARE_ERROR_CATEGORY(XiIoErrorCategory, "xi") {
      auto ec = (Enumeration)code;
      switch(ec) {
      case kEof:    return "End of file";
      case kUnableToReadMessageHeader:    return "Unable to read or parse message header";
      default: return "Unknown io";
      }
    }

    XI_DECLARE_STANDARD_ERROR_FUNCTIONS(XiIoErrorCategory);

    } //namespace error
  } //namespace io
} //namespace xi

XI_DECLARE_ERROR_ENUM(xi::io::error::Enumeration)
