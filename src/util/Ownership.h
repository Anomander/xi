#pragma once

#include "ext/Pointer.h"
#include "ext/TypeTraits.h"
#include "ext/Meta.h"

namespace xi { 
  inline namespace util {

           namespace ownership {

             enum class SharedPolicy {
               kArc,
               kRc,
               kStd,
             };
             /// It might be worth taking the actual class
             /// and verifying that it doesn't declare a
             /// different ownership mode somewhere in its
             /// superclasses, but for now this should
             /// suffice
             template <SharedPolicy> struct Shared {};
             template <> struct Shared <SharedPolicy::kStd>
               : public ::std::enable_shared_from_this <Shared <SharedPolicy::kStd>>
             {
               auto self() { return shared_from_this(); }
             };
             template <> struct Shared <SharedPolicy::kRc>
               : public RcCounter <Shared <SharedPolicy::kRc>>
             {
               auto self() { return intrusive_ptr<Shared <SharedPolicy::kRc>> (this); }
             };
             template <> struct Shared <SharedPolicy::kArc>
               : public ArcCounter <Shared <SharedPolicy::kArc>>
             {
               auto self() { return intrusive_ptr<Shared <SharedPolicy::kArc>> (this); }
             };
             struct Unique {};

             using StdShared = Shared <SharedPolicy::kStd>;
             using RcShared = Shared <SharedPolicy::kRc>;
             using ArcShared = Shared <SharedPolicy::kArc>;
           } // namespace ownership

           namespace detail {
             template<class T, ownership::SharedPolicy P>
             using EnableIfShared = meta::enable_if <is_base_of <ownership::Shared<P>, T>>;

             template<class T>
             using EnableIfUnique = meta::enable_if <is_base_of <ownership::Unique, T>>;

             template <class T> struct ensureNotConst {
               using type = remove_reference_t <remove_pointer_t <T>>;
               static_assert (! is_const <type> ::value, "T must not be const" ); 
             };
             template <class T> using ensureNotConst_t = typename ensureNotConst <T> ::type;

             template <class T, class ... > struct own { using type = remove_pointer_t < remove_reference_t <T>>; };
             template <class T, class ... > struct ref { using type = add_lvalue_reference_t <add_const_t <remove_reference_t <T>>>; };
             template <class T, class ... > struct mut { using type = add_pointer_t <ensureNotConst_t <T>>; };

             template <class T> struct ref <unique_ptr <T>> : ref <T> {};
             template <class T> struct ref <shared_ptr <T>> : ref <T> {};
             template <class T> struct ref <intrusive_ptr <T>> : ref <T> {};

             template <class T> struct mut <unique_ptr <T>> : mut <T> {};
             template <class T> struct mut <shared_ptr <T>> : mut <T> {};
             template <class T> struct mut <intrusive_ptr <T>> : mut <T> {};

             template <class T> struct own <T, EnableIfShared <T, ownership::SharedPolicy::kArc>>
             { using type = intrusive_ptr <T>; };
             template <class T> struct own <T, EnableIfShared <T, ownership::SharedPolicy::kRc>>
             { using type = intrusive_ptr <T>; };
             template <class T> struct own <T, EnableIfShared <T, ownership::SharedPolicy::kStd>>
             { using type = shared_ptr <T>; };
             template <class T> struct own <T, EnableIfUnique <T>> { using type = unique_ptr <T>; };

             template <class T, class ...>
             struct Maker {
               template <class ... Args>
               static auto make (Args && ... args) {
                 return T {::std::forward <Args> (args)... };
               }
             };
             template <class T>
             struct Maker <T, EnableIfShared <T, ownership::SharedPolicy::kStd>> {
               template <class ... Args>
                 static auto make (Args && ... args) {
                 return make_shared <T> (::std::forward <Args> (args)...);
               }
             };
             template <class T>
             struct Maker <T, EnableIfShared <T, ownership::SharedPolicy::kRc>> {
               template <class ... Args>
                 static auto make (Args && ... args) {
                 return intrusive_ptr <T> (new T (::std::forward <Args> (args)...));
               }
             };
             template <class T>
             struct Maker <T, EnableIfShared <T, ownership::SharedPolicy::kArc>> {
               template <class ... Args>
                 static auto make (Args && ... args) {
                 return intrusive_ptr <T> (new T (::std::forward <Args> (args)...));
               }
             };
             template <class T>
             struct Maker <T, EnableIfUnique <T>> {
               template <class ... Args>
                 static auto make (Args && ... args) {
                 return make_unique <T> (::std::forward <Args> (args)...);
               }
             };
           } // namespace detail

           template <class T> using own = typename detail::own <T, meta::enabled> ::type;
           template <class T> using ref = typename detail::ref <T, meta::enabled> ::type;
           template <class T> using mut = typename detail::mut <T, meta::enabled> ::type;

           template <class T, class ... Args> own <T> make(Args && ... args) {
             return detail::Maker <T, meta::enabled> ::make(::std::forward <Args> (args)...);
           }

           template <class T> inline own<T> copy(T && t) { return forward <T> (t); }
           template <class T> inline ref<T> cref(T && t) { return ref <T> (t); }
           template <class T> inline ref<T> cref(shared_ptr<T> t) { return ref <T> (* t); }
           template <class T> inline ref<T> cref(intrusive_ptr<T> t) { return ref <T> (* t); }
           template <class T> inline ref<T> cref(unique_ptr<T> & t) { return ref <T> (* t); }
           template <class T> inline ref<T> cref(unique_ptr<T> const & t) { return ref <T> (* t); }
           template <class T> inline mut<T> edit(T && t) { return mut <T> (addressOf(t)); }
           template <class T> inline mut<T> edit(shared_ptr<T> t) { return mut <T> (t.get()); }
           template <class T> inline mut<T> edit(intrusive_ptr<T> t) { return mut <T> (t.get()); }
           template <class T> inline mut<T> edit(unique_ptr<T> & t) { return mut <T> (t.get()); }

           template <class T> inline decltype(auto) val(T && t) { return t; }
           template <class T> inline decltype(auto) val(T * t) { return *t; }
           template <class T> inline decltype(auto) val(shared_ptr<T> const & t) { return *t; }
           template <class T> inline decltype(auto) val(intrusive_ptr<T> const & t) { return *t; }
           template <class T> inline decltype(auto) val(unique_ptr<T> const & t) { return *t; }
           template <class T> inline decltype(auto) val(unique_ptr<T> & t) { return *t; }

  } // inline namespace util
} // namespace xi
