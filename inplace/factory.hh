#ifndef INCLUDED_INPLACE_FACTORY_HH
#define INCLUDED_INPLACE_FACTORY_HH

#include "copy_move_semantics.hh"

#include <cassert>
#include <type_traits>
#include <utility>

namespace inplace {
  namespace detail {
    template<template<typename> class... input_traits>
    struct trait_or {
      template<typename T> using trait = std::disjunction<input_traits<T>...>;
      template<typename T> static constexpr bool trait_v = trait<T>::value;
    };

    template<typename... possible_types>
    using require_copy_semantics = std::conjunction<std::is_copy_constructible<possible_types>...>;

    template<typename... possible_types>
    using offer_move_semantics = std::conjunction<trait_or<std::is_move_constructible,
                                                           std::is_copy_constructible>::template trait<possible_types>...>;

    template<typename... possible_types>
    using require_move_semantics = std::conjunction<offer_move_semantics<possible_types...>,
                                                    std::disjunction<std::is_move_constructible<possible_types>...>>;
  }

  template<typename    base_type,
           typename... possible_types>
  class factory_base
    // EBO, wenn möglich.
    : private detail::copy_move_semantics<factory_base<base_type, possible_types...>,
                                          detail::require_copy_semantics<possible_types...>::value,
                                          detail::require_move_semantics<possible_types...>::value>
  {
    static_assert(sizeof...(possible_types) > 0, "possible_types is empty");
    static_assert(std::conjunction_v<std::is_base_of<base_type, possible_types>...>, "base_type is not base class of all possible_types");

  public:
    factory_base() noexcept = default;

    factory_base(factory_base const &other) { *this =                            other ; }
    factory_base(factory_base      &&other) { *this = std::forward<factory_base>(other); }

    // operator= kann nicht sinnvoll operator= des Werttyps benutzen, weil dieser in den beiden Operanden verschieden sein kann.
    factory_base& operator=(factory_base const &other) {
      if(&other != this) {
        other.cpmov_type::do_copy(other , *this);
      }

      return *this;
    }

    factory_base& operator=(factory_base &&other) {
      if(&other != this) {
        other.cpmov_type::do_move(std::forward<factory_base>(other), *this);
        other.clear();
      }

      return *this;
    }

    ~factory_base() noexcept {
      clear();
    }

    void clear() noexcept {
      if(obj_ptr_) {
        obj_ptr_->~base_type();
        obj_ptr_ = nullptr;
        cpmov_type::clear();
      }
    }

    template<typename T, typename... Args>
    void construct(Args&&... args) {
      static_assert(std::disjunction_v<std::is_same<T, possible_types>...>, "attempted to construct invalid type in inplace::factory_base");

      clear();
      construct_backend<T>::construct(storage(), std::forward<Args>(args)...);

      obj_ptr_ = static_cast<T*>(storage());
      cpmov_type::template set_type<T>();
    }

    bool is_initialized() const noexcept { return get_ptr() != nullptr; }

    base_type *get_ptr() const noexcept {
      return obj_ptr_;
    }

    base_type &get() const noexcept {
      assert(get_ptr() != nullptr);
      return *get_ptr();
    }

    base_type *operator->() const noexcept { return get_ptr(); }
    base_type &operator* () const noexcept { return get    (); }

    explicit operator bool() const noexcept { return  get_ptr(); }
    bool operator!() const noexcept { return !is_initialized(); }

  private:
    template<typename T, bool, bool> friend struct detail::copy_move_semantics;

    template<typename T, bool = std::is_move_constructible<T>::value>
    struct construct_backend {
      template<typename... Args>
      static void construct(void *place, Args&&... args) {
        new(place) T(std::forward<Args>(args)...);
      }
    };

    // Move-Semantik für nicht-bewegbare Typen möglich machen: fallback auf copy.
    template<typename T>
    struct construct_backend<T, false> {
      template<typename... Args>
      static void construct(void *place, Args&&... args) {
        new(place) T(std::forward<Args>(args)...);
      }
      static void construct(void *place, T &&other) {
        new(place) T(other);
      }
    };

    // TODO: Durch std::aligned_union ersetzen, sobald libstdc++ das hat.
    using storage_type = std::aligned_union_t<1, possible_types...>;
    using cpmov_type = detail::copy_move_semantics<factory_base,
                                                   detail::require_copy_semantics<possible_types...>::value,
                                                   detail::require_move_semantics<possible_types...>::value>;

    void       *storage()       noexcept { return static_cast<void       *>(&storage_); }
    void const *storage() const noexcept { return static_cast<void const *>(&storage_); }

    storage_type  storage_;
    base_type    *obj_ptr_ = nullptr;
  };

  template<bool copy, bool move>
  struct factory_ctors_controller {
  };

  template<> struct factory_ctors_controller<false, false> {
    factory_ctors_controller() = default;
    factory_ctors_controller(factory_ctors_controller const &) = delete;
    factory_ctors_controller(factory_ctors_controller      &&) = delete;

    factory_ctors_controller &operator=(factory_ctors_controller const &) = delete;
    factory_ctors_controller &operator=(factory_ctors_controller      &&) = delete;
  };

  template<> struct factory_ctors_controller<false, true> {
    factory_ctors_controller() = default;
    factory_ctors_controller(factory_ctors_controller const &) = delete;
    factory_ctors_controller(factory_ctors_controller      &&) = default;

    factory_ctors_controller &operator=(factory_ctors_controller const &) = delete;
    factory_ctors_controller &operator=(factory_ctors_controller      &&) = default;
  };

  template<typename    base_type,
           typename... possible_types>
  class factory : public factory_base<base_type, possible_types...>,
                  private factory_ctors_controller<detail::require_copy_semantics<possible_types...>::value,
                                                   detail::offer_move_semantics  <possible_types...>::value>
  {
  public:
    factory() = default;

    template<typename T, typename... Args>
    factory(T f, Args&&... args) {
      f(*this, std::forward<Args>(args)...);
    }
  };
}

#endif
