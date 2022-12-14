#ifndef INCLUDED_INPLACE_FACTORY_HH
#define INCLUDED_INPLACE_FACTORY_HH

#include "copy_move_semantics.hh"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace inplace {
  // In-place factory, i.e. sort of a polymorphic variant.
  //
  // This is useful when the overhead of dynamic allocation has to be avoided but runtime polymorphy
  // is still desired.
  template<typename base_type, std::derived_from<base_type>... possible_types>
  class factory {
    static_assert(sizeof...(possible_types) > 0, "possible_types is empty");

  private:
    using cpmov = detail::copy_move_traits<possible_types...>;

    template<typename T>
    static constexpr bool allowed_type = std::disjunction_v<std::is_same<T, possible_types>...>;

  public:
    factory() noexcept = default;

    factory(factory const &other) requires cpmov::offer_copy {
      *this = other;
    }

    factory(factory &&other) requires cpmov::offer_move {
      *this = std::forward<factory>(other);
    }

    template<typename... Args>
    factory(std::invocable<factory&, Args...> auto &&f, Args&&... args) {
      f(*this, std::forward<Args>(args)...);
    }

    // operator= cannot sensibly use the value types; operator= because the other factory may contain a different type,
    // so we always use constructors for assignment.
    factory &operator=(factory const &other) requires cpmov::offer_copy {
      if(&other != this) {
        other.cpmov_handler_.do_copy(other, *this);
      }

      return *this;
    }

    factory &operator=(factory &&other) requires cpmov::offer_move  {
      if(&other != this) {
        other.cpmov_handler_.do_move(std::forward<factory>(other), *this);
        other.clear();
      }

      return *this;
    }

    ~factory() noexcept {
      clear();
    }

    void clear() noexcept {
      if(is_initialized()) {
        obj_ptr_->~base_type();
        obj_ptr_ = nullptr;
        cpmov_handler_.clear();
      }
    }

    template<typename T, typename... Args>
    requires allowed_type<T>
    base_type *construct(Args&&... args) {
      clear();
      obj_ptr_ = do_construct<T>(std::forward<Args>(args)...);
      cpmov_handler_.template set_type<T>();

      return obj_ptr_;
    }

    bool is_initialized() const noexcept {
      return get_ptr() != nullptr;
    }

    base_type *get_ptr() const noexcept {
      return obj_ptr_;
    }

    base_type &get() const noexcept {
      assert(get_ptr() != nullptr);
      return *get_ptr();
    }

    base_type *operator->() const noexcept {
      return get_ptr();
    }

    base_type &operator*() const noexcept {
      return get();
    }

    explicit operator bool() const noexcept {
      return is_initialized();
    }

  private:
    template<typename T, bool, bool> friend struct detail::copy_move_semantics;

    template<typename T, typename... Args>
    T *do_construct(Args&&... args) {
      return new(storage()) T(std::forward<Args>(args)...);
    }

    // For non-moveable types: move construction falls back to copy
    template<typename T>
    T *do_construct(T &&other) requires (!std::is_move_constructible_v<T>) {
      return new(storage()) T(other);
    };

    void       *storage()       noexcept { return storage_; }
    void const *storage() const noexcept { return storage_; }

    alignas(possible_types...)
    std::byte storage_[std::max({sizeof(possible_types)...})];

    // pointer-to-base referencing the object constructed in storage_. This is necessary
    // because of multiple inheritance: if base_type is not the concrete type's first base
    // class, then static_cast<base_type*>(storage()) will give the wrong address.
    base_type *obj_ptr_ = nullptr;

    // cpmov_handler_ is empty when neither copy nor move are supported.
    [[no_unique_address]]
    detail::copy_move_semantics<factory, cpmov::require_copy, cpmov::require_move> cpmov_handler_;
  };
}

#endif
