#ifndef INCLUDED_INPLACE_FACTORY_HH
#define INCLUDED_INPLACE_FACTORY_HH

#include "copy_move_semantics.hh"

#include <cassert>
#include <concepts>
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
    template<typename T> static constexpr bool allowed_type = std::disjunction_v<std::is_same<T, possible_types>...>;

  public:
    factory() noexcept = default;

    factory(factory const &other) requires detail::offer_copy_semantics_v<possible_types...> {
      *this = other;
    }

    factory(factory &&other) requires detail::offer_move_semantics_v<possible_types...> {
      *this = std::forward<factory>(other);
    }

    template<typename... Args>
    factory(std::invocable<factory&, Args...> auto &&f, Args&&... args) {
      f(*this, std::forward<Args>(args)...);
    }

    // operator= cannot sensibly use the value types; operator= because the other factory may contain a different type,
    // so we always use constructors for assignment.
    factory &operator=(factory const &other) requires detail::offer_copy_semantics_v<possible_types...> {
      if(&other != this) {
        other.cpmov_handler_.do_copy(other, *this);
      }

      return *this;
    }

    factory &operator=(factory &&other) requires detail::offer_move_semantics_v<possible_types...>  {
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
      if(obj_ptr_) {
        obj_ptr_->~base_type();
        obj_ptr_ = nullptr;
        cpmov_handler_.clear();
      }
    }

    template<typename T, typename... Args>
    requires allowed_type<T>
    void construct(Args&&... args) {
      clear();
      construct_backend<T>::construct(storage(), std::forward<Args>(args)...);

      obj_ptr_ = static_cast<T*>(storage());
      cpmov_handler_.template set_type<T>();
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

    template<typename T>
    struct construct_backend {
      template<typename... Args>
      static void construct(void *place, Args&&... args) {
        new(place) T(std::forward<Args>(args)...);
      }

      // For non-moveable types: move construction falls back to copy
      static void construct(void *place, T &&other) requires (!std::is_move_constructible_v<T>) {
        new(place) T(other);
      }
    };

    using storage_type = std::aligned_union_t<1, possible_types...>;
    using cpmov_type = detail::copy_move_semantics<factory,
                                                   detail::require_copy_semantics_v<possible_types...>,
                                                   detail::require_move_semantics_v<possible_types...>>;

    void       *storage()       noexcept { return static_cast<void       *>(&storage_); }
    void const *storage() const noexcept { return static_cast<void const *>(&storage_); }

    storage_type  storage_;
    base_type    *obj_ptr_ = nullptr;

    // cpmov_handler_ is empty when neither copy nor move are supported.
    [[no_unique_address]] cpmov_type cpmov_handler_;
  };
}

#endif
