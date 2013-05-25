#ifndef INCLUDED_INPLACE_FACTORY_HH
#define INCLUDED_INPLACE_FACTORY_HH

#include "geometry.hh"
#include "copy_move_semantics.hh"
#include "pack_traits.hh"

#include <cassert>
#include <type_traits>
#include <utility>

namespace inplace {
  namespace detail {
    template<typename... possible_types> struct require_copy_semantics {
      static bool const value = pack::applies_to_all<std::is_copy_constructible, possible_types...>::value;
    };

    template<typename... possible_types> struct offer_move_semantics {
      static bool const value = pack::applies_to_all<pack::trait_or_reduce<std::is_move_constructible, std::is_copy_constructible>::template trait, possible_types...>::value;
    };

    template<typename... possible_types> struct require_move_semantics {
      static bool const value = offer_move_semantics<possible_types...>::value &&
                                pack::applies_to_any<std::is_move_constructible, possible_types...>::value;
    };
  }

  template<typename    base_type,
           typename... possible_types>
  class factory_base
    // EBO, wenn möglich.
    : private detail::copy_move_semantics<factory_base<base_type, possible_types...>,
                                          detail::require_copy_semantics<possible_types...>::value,
                                          detail::require_move_semantics<possible_types...>::value>
  {
    static_assert(sizeof...(possible_types) > 0, "possible_types ist leer");
    static_assert(pack::is_base_of_all<base_type, possible_types...>::value, "base_type ist nicht Basisklasse aller possible_types");

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
      static_assert(pack::contains<T, possible_types...>::value, "Unzulaessiger Typ für inplace::factory_base");

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
    typedef detail::geometry<possible_types...>                           geometry_type;
    typedef typename std::aligned_storage<geometry_type::space,
                                          geometry_type::alignment>::type storage_type;
    typedef detail::copy_move_semantics<factory_base,
                                        detail::require_copy_semantics<possible_types...>::value,
                                        detail::require_move_semantics<possible_types...>::value> cpmov_type;

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
