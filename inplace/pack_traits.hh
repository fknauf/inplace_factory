#ifndef INCLUDED_PACK_TRAITS_HH
#define INCLUDED_PACK_TRAITS_HH

#include <type_traits>

namespace pack {
  ////////////////////////////////
  // contains
  ////////////////////////////////

  template<typename, typename...> struct contains;

  template<typename T, typename U, typename... Pack>
  struct contains<T, U, Pack...> {
    static bool const value = std::is_same<T, U>::value || contains<T, Pack...>::value;
  };

  template<typename T>
  struct contains<T> {
    static bool const value = false;
  };

  ////////////////////////////////
  // applies_to_all
  ////////////////////////////////

  template<template<typename> class, typename...> struct applies_to_all;

  template<template<typename> class predicate, typename T, typename... Pack>
  struct applies_to_all<predicate, T, Pack...> {
    static bool const value = predicate<T>::value && applies_to_all<predicate, Pack...>::value;
  };

  template<template<typename> class predicate>
  struct applies_to_all<predicate> {
    static bool const value = true;
  };

  ////////////////////////////////
  // is_base_of_all
  ////////////////////////////////

  template<template<typename, typename> class binary_trait, typename T> struct trait_bind1st {
    template<typename U> struct trait {
      static bool const value = binary_trait<T, U>::value;
    };
  };

  template<typename base, typename... types> struct is_base_of_all {
    static bool const value = applies_to_all<trait_bind1st<std::is_base_of, base>::template trait, types...>::value;
  };

  ////////////////////////////////
  // trait_or_reduce
  ////////////////////////////////

  template<template<typename> class...>
  struct trait_or_reduce {
    template<typename T> struct trait {
      static bool const value = false;
    };
  };

  template<template<typename> class    front_condition,
           template<typename> class... tail_conditions>
  struct trait_or_reduce<front_condition, tail_conditions...> {
    template<typename T> struct trait {
      static bool const value = front_condition<T>::value || trait_or_reduce<tail_conditions...>::template trait<T>::value;
    };
  };
}

#endif
