#pragma once

#include <memory>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

namespace xi { 
  inline namespace ext {

           using ::std::shared_ptr;
           using ::std::unique_ptr;
           using ::std::make_shared;
           using ::std::make_unique;
           using ::std::enable_shared_from_this;

           using ::std::static_pointer_cast;
           using ::std::dynamic_pointer_cast;

           using ::boost::intrusive_ptr;
           template <class T> using RcCounter = ::boost::intrusive_ref_counter <T, ::boost::thread_unsafe_counter>;
           template <class T> using ArcCounter = ::boost::intrusive_ref_counter <T, ::boost::thread_safe_counter>;

           template<class T, class U>
           auto dynamic_pointer_cast (unique_ptr <U> && ptr) noexcept {
             auto cast = dynamic_cast<T*> (ptr.get());
             if (cast) { ptr.release(); }
             return unique_ptr<T> (cast);
           }

  } // inline namespace ext
} // namespace xi
