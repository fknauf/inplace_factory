#ifndef INCLUDED_INPLACE_DETAIL_GEOMETRY_HH
#define INCLUDED_INPLACE_DETAIL_GEOMETRY_HH

#include "const_algo.hh"
#include <cstddef>

namespace inplace {
  namespace detail {
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
  }
}

#endif
