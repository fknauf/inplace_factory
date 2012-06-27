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
  // is_base_of_all
  ////////////////////////////////

  template<typename, typename...> struct is_base_of_all;

  template<typename T, typename U, typename... Pack>
  struct is_base_of_all<T, U, Pack...> {
    static bool const value = std::is_base_of<T, U>::value && is_base_of_all<T, Pack...>::value;
  };

  template<typename T, typename U>
  struct is_base_of_all<T, U> {
    static bool const value = std::is_base_of<T, U>::value;
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
}

#endif
