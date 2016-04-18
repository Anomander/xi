#pragma once

#include "xi/ext/configure.h"
#include <boost/program_options.hpp>

namespace xi {
namespace core {
  class configuration {
  public:
    class backend;
    class value;

  private:
    unique_ptr< backend > _backend;

  public:
    template < class Backend >
    configuration& init();
    void read_command_line(int, char* []);
    void read_file(string_ref);
    mut< value > get_value(string_ref);
  };

  class configuration::backend : public virtual ownership::unique {
  public:
    XI_CLASS_DEFAULTS(backend, no_copy, no_move, ctor, virtual_dtor);
    virtual void read_command_line(int, char* []) = 0;
    virtual void read_file(string_ref)           = 0;
    virtual opt< u64 > get_ulong(string_ref)     = 0;
    virtual opt< u32 > get_uint(string_ref)      = 0;
    virtual opt< u8 > get_byte(string_ref)       = 0;
    virtual opt< double > get_double(string_ref) = 0;
    virtual opt< string > get_string(string_ref) = 0;
  };

  template < class Backend >
  configuration& configuration::init() {
    static_assert(is_base_of< backend, Backend >::value, "TODO");
    assert(!_backend);
    _backend = make< Backend >();
    return *this;
  }

  inline void configuration::read_command_line(int argc, char* argv[]) {
    _backend->read_command_line(argc, argv);
  }

  inline void configuration::read_file(string_ref name) {
    _backend->read_file(name);
  }

  class boost_po_backend : public configuration::backend {
    boost::program_options::variables_map _config;
    boost::program_options::options_description _options;

  private:
    template < class T >
    opt< T > get_typed_value(string_ref name) {
      auto&& config = _config[name.to_string()];
      if (config.empty()) {
        return none;
      }
      auto&& any  = config.value();
      auto result = boost::any_cast< T >(&any);
      return result ? some(*result) : none;
    }

  public:
    boost_po_backend() : _options("Configuration") {
      _options.add_options()("help,h", "Show help");
    }

    void read_command_line(int argc, char* argv[]) override {
      boost::program_options::store(
          boost::program_options::command_line_parser(argc, argv)
              .options(_options)
              .run(),
          _config);
    }

    void read_file(string_ref) override {
    }

    opt< u64 > get_ulong(string_ref name) override {
      return get_typed_value< u64 >(name);
    }
    opt< u32 > get_uint(string_ref name) override {
      return get_typed_value< u32 >(name);
    }
    opt< u8 > get_byte(string_ref name) override {
      return get_typed_value< u8 >(name);
    }
    opt< double > get_double(string_ref name) override {
      return get_typed_value< double >(name);
    }
    opt< string > get_string(string_ref name) override {
      return get_typed_value< string >(name);
    }
  };
}
}
