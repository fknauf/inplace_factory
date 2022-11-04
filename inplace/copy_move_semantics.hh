#ifndef INCLUDED_COPY_MOVE_SEMANTICS_HH
#define INCLUDED_COPY_MOVE_SEMANTICS_HH

#include <type_traits>
#include <utility>

// Copy/Move semantics encapsulation for the factory type.
// This is a little bit messy because the set of possible types may contain any mixture of
// types that support or don't support copy and/or move construction.
//
// Generally, we try to support the maximum possible set of operations. That is:
//
// 1. Support copy construction only if all types support copy construction
// 2. Support move construction if all types support copy or move construction
// 3. Fall back on copy construction if the factory supports move but the concrete
//    type in it only supports copy construction

namespace inplace {
  namespace detail {
    template<template<typename> class... input_traits>
    struct trait_or {
      template<typename T> using trait = std::disjunction<input_traits<T>...>;
    };

    // Type-traits to decide whether and how to offer copy/move semantics.
    // Offer: What the factory supports from the outside
    // Require: what the copy_move_semantics in the background must handle.
    template<typename... possible_types>
    struct copy_move_traits {
      // Copy: Offered when all possible types support copy; required in the same case.
      static bool constexpr offer_copy   = std::conjunction_v<std::is_copy_constructible<possible_types>...>;
      static bool constexpr require_copy = offer_copy;

      // Move: Offered when all possible types support move or copy, required when at least one type supports move.
      static bool constexpr offer_move   = std::conjunction_v<trait_or<std::is_move_constructible,
                                                                       std::is_copy_constructible>::template trait<possible_types>...>;
      static bool constexpr require_move = offer_move && std::disjunction_v<std::is_move_constructible<possible_types>...>;
    };

    // Not all types support copy, not all types support move
    // Factory supports neither copy or move.
    //
    // copy_move_semantics does nothing.
    template<typename factory_type,
             bool enable_copy,
             bool enable_move>
    struct copy_move_semantics {
      template<typename T> requires factory_type::template allowed_type<T>
      void set_type() { }
      void clear   () { }
    };

    // All types support copy, no type supports move.
    // Factory will support move but internally use copy construction to achieve it.
    //
    // copy_move_semantics stores a function pointer to a type-specific copy function.
    template<typename factory_type> class copy_move_semantics<factory_type, true, false> {
    public:
      template<typename T> requires factory_type::template allowed_type<T>
      void set_type() { do_copy_ptr = &copy_move_semantics::copy_impl<T>; }
      void clear   () { do_copy_ptr = &copy_move_semantics::copy_empty  ; }

      void do_copy(factory_type const &from, factory_type &to) const { do_copy_ptr(from, to); }
      // fallback to copy if move semantics are unavailable
      void do_move(factory_type      &&from, factory_type &to) const { do_copy_ptr(from, to); }

    private:
      void (*do_copy_ptr)(factory_type const &from, factory_type &to) = copy_empty;

      template<typename T> requires factory_type::template allowed_type<T>
      static void copy_impl (factory_type const &from, factory_type &to) { to.template construct<T>(*static_cast<T const *>(from.storage())); }
      static void copy_empty(factory_type const &    , factory_type &to) { to.clear(); } // copy from an empty factory.
    };

    // All types support move, not all types support copy
    // Factory will support move but not copy.
    //
    // copy_move_semantics stores a function pointer to a type-specific move function.
    template<typename factory_type> class copy_move_semantics<factory_type, false, true> {
    public:
      template<typename T> requires factory_type::template allowed_type<T>
      void set_type() { do_move_ptr = &copy_move_semantics::move_impl<T>; }
      void clear   () { do_move_ptr = &copy_move_semantics::move_empty  ; }

      void do_move(factory_type &&from, factory_type &to) const { do_move_ptr(std::forward<factory_type>(from), to); }

    private:
      void (*do_move_ptr)(factory_type &&from, factory_type &to) = move_empty;

      template<typename T> requires factory_type::template allowed_type<T>
      static void move_impl (factory_type &&from, factory_type &to) { to.template construct<T>(std::move(*static_cast<T *>(from.storage()))); }
      static void move_empty(factory_type &&    , factory_type &to) { to.clear(); } // move from an empty factory
    };

    // All types support copy, some types support move.
    // Factory supports copy and move, and it will use proper move semantics when available.
    //
    // copy_move_semantics uses the copy-only and move-only cases of itself to provide both operations
    template<typename factory_type> class copy_move_semantics<factory_type, true, true> {
    public:
      void clear() {
        cs_.clear();
        ms_.clear();
      }

      template<typename T> requires factory_type::template allowed_type<T>
      void set_type() {
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
