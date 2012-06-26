#ifndef INCLUDED_INPLACE_FACTORY_HH
#define INCLUDED_INPLACE_FACTORY_HH

#include "const_algo.hh"
#include "pack_traits.hh"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace inplace {
  template<typename... types> struct geometry;

  template<typename head_type, typename... tail_types>
  struct geometry<head_type, tail_types...> {
    static std::size_t const space     = const_algo::max( sizeof(head_type), geometry<tail_types...>::space    );
    static std::size_t const alignment = const_algo::lcm(alignof(head_type), geometry<tail_types...>::alignment);
  };

  template<>
  struct geometry<> {
    static std::size_t const space     = 0;
    static std::size_t const alignment = 1;
  };

  template<typename factory_type, bool> struct copy_semantics {
    template<typename T>
    static void copy(factory_type const &, factory_type &) {
      throw std::logic_error("inplace::factory: Kopiersemantik nicht verfuegbar");
    }
  };

  template<typename factory_type>
  struct copy_semantics<factory_type, true> {
    template<typename T>
    static void copy(factory_type const &from,
                     factory_type       &to) {
      to.template construct<T>(*static_cast<T const *>(from.storage()));
    }
  };

  template<typename factory_type, bool> struct move_semantics {
    template<typename T>
    static void move(factory_type &&, factory_type &) {
      throw std::logic_error("inplace::factory: Movesemantik nicht verfuegbar");
    }
  };

  template<typename factory_type>
  struct move_semantics<factory_type, true> {
    template<typename T>
    static void move(factory_type &&from,
                     factory_type  &to) {
      to.template construct<T>(std::move(*static_cast<T *>(from.storage())));
    }
  };

  template<typename    base_type,
           typename... possible_types>
  struct factory {
    static_assert(sizeof...(possible_types) > 0, "possible_types ist leer");
    static_assert(pack::is_base_of_all<base_type, possible_types...>::value, "base_type ist nicht Basisklasse aller possible_types");

    static bool const has_copy_semantics = pack::applies_to_all<std::is_copy_constructible, possible_types...>::value;
    static bool const has_move_semantics = pack::applies_to_all<std::is_move_constructible, possible_types...>::value;

  public:
    factory() noexcept = default;

    template<typename T, typename... Args>
    factory(T f, Args&&... args) {
      f(*this, std::forward<Args>(args)...);
    }

    factory(typename std::enable_if<has_copy_semantics, factory>::type const &other) { other.copy_to(*this); }
    factory(typename std::enable_if<has_move_semantics, factory>::type      &&other) { other.move_to(*this); }

    typename std::enable_if<has_copy_semantics, factory>::type &operator=(factory const &other) { other.copy_to(*this); return *this; }
    typename std::enable_if<has_move_semantics, factory>::type &operator=(factory      &&other) { other.move_to(*this); return *this; }

    ~factory() noexcept {
      reset();
    }

    void reset() noexcept {
      if(obj_ptr_) {
        obj_ptr_->~base_type();
        obj_ptr_ = 0;
        copy_to_backend = &factory::copy_to_dummy;
        move_to_backend = &factory::move_to_dummy;
      }
    }

    template<typename T, typename... Args>
    void construct(Args&&... args) {
      static_assert(pack::contains<T, possible_types...>::value, "Unzulaessiger Typ für inplace::factory");

      reset();
      new(storage()) T(std::forward<Args>(args)...);

      obj_ptr_ = static_cast<T*>(storage());
      copy_to_backend = &copy_semantics<factory, has_copy_semantics>::template copy<T>;
      move_to_backend = &move_semantics<factory, has_move_semantics>::template move<T>;
    }

    bool is_initialized() const noexcept { return get_ptr() != 0; }

    base_type *get_ptr() const noexcept {
      return obj_ptr_;
    }

    base_type &get() const noexcept {
      assert(get_ptr() != 0);
      return *get_ptr();
    }

    base_type *operator->() const noexcept { return get_ptr(); }
    base_type &operator* () const noexcept { return get    (); }

    operator  void*() const noexcept { return  get_ptr(); }
    bool operator !() const noexcept { return !is_initialized(); }

  private:
    friend class copy_semantics<factory, true>;
    friend class move_semantics<factory, true>;

    typedef geometry<possible_types...>                                   geometry_type;
    typedef typename std::aligned_storage<geometry_type::space,
                                          geometry_type::alignment>::type storage_type;

    void       *storage()       noexcept { return static_cast<void*>(&storage_); }
    void const *storage() const noexcept { return static_cast<void const *>(&storage_); }

    void copy_to(factory &other) const { copy_to_backend(*this, other); }
    void move_to(factory &other)       { move_to_backend(std::move(*this), other); }

    static void copy_to_dummy(factory const &, factory &) {
      throw std::logic_error("inplace::factory enthaelt kein Objekt");
    }

    static void move_to_dummy(factory &&, factory &) {
      throw std::logic_error("inplace::factory enthaelt kein Objekt");
    }

    storage_type  storage_;
    base_type    *obj_ptr_ = 0;

    void (*copy_to_backend)(factory const&, factory &) = &factory::copy_to_dummy;
    void (*move_to_backend)(factory     &&, factory &) = &factory::move_to_dummy;
  };
}

#endif
