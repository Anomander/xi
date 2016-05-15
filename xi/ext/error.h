#pragma once

#include "xi/ext/string.h"

#include <system_error>

namespace xi {
inline namespace ext {

  using ::std::error_code;
  using ::std::error_category;
  using ::std::error_condition;

  using ::std::system_error;
  using ::std::system_category;
  using ::std::generic_category;

  using ::std::is_error_code_enum;

  namespace detail {
    class undefined_boost_error_category final : public error_category {
    public:
      const char *name() const noexcept override {
        return "Undefined boost error category";
      }
      string message(int ev) const override {
        return "Undefined boost error " + to_string(ev);
      }
      error_condition default_error_condition(int ev) const noexcept override {
        return error_condition{ev, *this};
      };
    };
  }

  inline auto error_from_errno() {
    return error_code(errno, generic_category());
  }

  inline auto error_from_value(int value) {
    return error_code(value, generic_category());
  }

#define XI_DECLARE_ERROR_ENUM(E)                                               \
  namespace std {                                                              \
    template <>                                                                \
    struct is_error_code_enum< E > : true_type {};                             \
  }

#define XI_DECLARE_ERROR_CATEGORY(C, Name)                                     \
  struct C : error_category {                                                  \
    const char *name() const noexcept override {                               \
      return Name;                                                             \
    }                                                                          \
    std::error_condition default_error_condition(int e) const                  \
        noexcept override {                                                    \
      return std::error_condition(e, *this);                                   \
    }                                                                          \
    std::string message(int code) const override;                              \
    static std::error_category const &instance();                              \
  };                                                                           \
  inline std::error_category const &C::instance() {                            \
    static C inst;                                                             \
    return inst;                                                               \
  }                                                                            \
  inline std::string C::message(int code) const

#define XI_DECLARE_STANDARD_ERROR_FUNCTIONS(C)                                 \
  inline std::error_code make_error_code(enumeration value) {                  \
    return std::error_code((int)value, C::instance());                         \
  }                                                                            \
                                                                               \
  inline std::error_condition make_error_condition(enumeration value) {        \
    return std::error_condition((int)value, C::instance());                    \
  }

  [[gnu::noreturn]] inline void throw_errno() {
    throw system_error(error_from_errno());
  }
} // inline namespace ext
} // namespace xi
