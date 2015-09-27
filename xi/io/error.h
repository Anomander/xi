#pragma once

#include "xi/ext/error.h"

namespace xi {
namespace io {
  namespace error {

    enum enumeration {
      kEOF,
      kUnableToReadMessageHeader,
    };

    XI_DECLARE_ERROR_CATEGORY(xi_io_error_category, "xi") {
      auto ec = (enumeration)code;
      switch (ec) {
        case kEOF:
          return "End of file";
        case kUnableToReadMessageHeader:
          return "Unable to read or parse message header";
        default:
          return "Unknown io";
      }
    }

    XI_DECLARE_STANDARD_ERROR_FUNCTIONS(xi_io_error_category);

  } // namespace error
} // namespace io
} // namespace xi

XI_DECLARE_ERROR_ENUM(xi::io::error::enumeration)
