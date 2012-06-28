#ifndef INCLUDED_INPLACE_FACTORY_HH
#define INCLUDED_INPLACE_FACTORY_HH

#include "const_algo.hh"
#include "pack_traits.hh"

#include <cassert>
#include <type_traits>
#include <utility>

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

  template<typename T, bool = T::has_copy_semantics, bool = T::has_move_semantics> struct copy_move_semantics {
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

  template<typename    base_type,
           typename... possible_types>
  struct factory {
    static_assert(sizeof...(possible_types) > 0, "possible_types ist leer");
    static_assert(pack::is_base_of_all<base_type, possible_types...>::value, "base_type ist nicht Basisklasse aller possible_types");

    static bool const has_copy_semantics = pack::applies_to_all<std::is_copy_constructible, possible_types...>::value;
    static bool const has_move_semantics = pack::applies_to_all<std::is_move_constructible, possible_types...>::value || has_copy_semantics;

  public:
    factory() noexcept = default;

    template<typename T, typename... Args>
    factory(T f, Args&&... args) {
      f(*this, std::forward<Args>(args)...);
    }

    factory(factory const &other) { other.cpmov_sem_.do_copy(                      other , *this);                }
    factory(factory     && other) { other.cpmov_sem_.do_move(std::forward<factory>(other), *this); other.clear(); }

    factory& operator=(factory const &other) { other.cpmov_sem_.do_copy(                      other , *this);                return *this; }
    factory& operator=(factory      &&other) { other.cpmov_sem_.do_move(std::forward<factory>(other), *this); other.clear(); return *this; }

    ~factory() noexcept {
      clear();
    }

    void clear() noexcept {
      if(obj_ptr_) {
        obj_ptr_->~base_type();
        obj_ptr_ = 0;
        cpmov_sem_.clear();
      }
    }

    template<typename T, typename... Args>
    void construct(Args&&... args) {
      static_assert(pack::contains<T, possible_types...>::value, "Unzulaessiger Typ für inplace::factory");

      clear();
      construct_backend<T>::construct(storage(), std::forward<Args>(args)...);

      obj_ptr_ = static_cast<T*>(storage());
      cpmov_sem_.template set_type<T>();
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
    template<typename T, bool, bool> friend class copy_move_semantics;

    template<typename T, bool = std::is_move_constructible<T>::value>
    struct construct_backend           { template<typename... Args> static void construct(void *place, Args&&... args) { new(place) T(std::forward<Args>(args)...); } };
    template<typename T>
    struct construct_backend<T, false> { template<typename... Args> static void construct(void *place, Args&&... args) { new(place) T(                   args ...); } };

    typedef geometry<possible_types...>                                   geometry_type;
    typedef typename std::aligned_storage<geometry_type::space,
                                          geometry_type::alignment>::type storage_type;

    void       *storage()       noexcept { return static_cast<void*>(&storage_); }
    void const *storage() const noexcept { return static_cast<void const *>(&storage_); }

    storage_type  storage_;
    base_type    *obj_ptr_ = 0;

    copy_move_semantics<factory> cpmov_sem_;
  };
}

#endif
