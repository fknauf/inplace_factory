#ifndef INCLUDED_COPY_MOVE_SEMANTICS_HH
#define INCLUDED_COPY_MOVE_SEMANTICS_HH

#include <utility>

namespace inplace {
  namespace detail {
    template<typename T, bool = T::needs_copy_semantics, bool = T::needs_move_semantics> struct copy_move_semantics {
      template<typename>
      void set_type() { }
      void clear   () { }
    };

    template<typename factory_type> class copy_move_semantics<factory_type, true, false> {
    public:
      template<typename T>
      void set_type() { do_copy_ptr = &copy_move_semantics::copy_impl<T>; }
      void clear   () { do_copy_ptr = &copy_move_semantics::copy_empty  ; }

      void do_copy(factory_type const &from, factory_type &to) const { do_copy_ptr(from, to); }
      // Wenn  move-Semantik nicht vorhanden ist, fallback auf copy.
      void do_move(factory_type      &&from, factory_type &to) const { do_copy_ptr(from, to); }

    private:
      void (*do_copy_ptr)(factory_type const &from, factory_type &to) = copy_empty;

      template<typename T>
      static void copy_impl (factory_type const &from, factory_type &to) { to.template construct<T>(*static_cast<T const *>(from.storage())); }
      static void copy_empty(factory_type const &    , factory_type &to) { to.clear(); }
    };

    template<typename factory_type> class copy_move_semantics<factory_type, false, true> {
    public:
      template<typename T>
      void set_type() { do_move_ptr = &copy_move_semantics::move_impl<T>; }
      void clear   () { do_move_ptr = &copy_move_semantics::move_empty  ; }

      void do_move(factory_type &&from, factory_type &to) const { do_move_ptr(std::forward<factory_type>(from), to); }

    private:
      void (*do_move_ptr)(factory_type &&from, factory_type &to) = move_empty;

      template<typename T>
      static void move_impl (factory_type &&from, factory_type &to) { to.template construct<T>(std::move(*static_cast<T *>(from.storage()))); }
      static void move_empty(factory_type &&    , factory_type &to) { to.clear(); }
    };

    template<typename factory_type> class copy_move_semantics<factory_type, true, true> {
    public:
      void clear() {
        cs_.clear();
        ms_.clear();
      }

      template<typename T> void set_type() {
        cs_.template set_type<T>();
        ms_.template set_type<T>();
      }

      void do_copy(factory_type const &from, factory_type &to) const { cs_.do_copy(                           from , to); }
      void do_move(factory_type      &&from, factory_type &to) const { ms_.do_move(std::forward<factory_type>(from), to); }

    private:
      copy_move_semantics<factory_type, true, false> cs_;
      copy_move_semantics<factory_type, false, true> ms_;
    };
  }
}

#endif
