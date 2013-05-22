#ifndef INCLUDED_CONST_ALGO_HH
#define INCLUDED_CONST_ALGO_HH

namespace const_algo {
  template<typename T> constexpr T max(T x, T y) { return x > y ? x : y; }
  template<typename T> constexpr T min(T x, T y) { return x < y ? x : y; }

  namespace impl {
    template<typename T>
    constexpr T lcm_helper(T upper, T lower, T interim) {
      return interim % lower == 0 ? interim : lcm_helper(upper, lower, interim + upper);
    }
  }

  template<typename T>
  constexpr T lcm(T x, T y) {
    return impl::lcm_helper(::const_algo::max(x, y),
                            ::const_algo::min(x, y),
                            ::const_algo::max(x, y));
  }
}

#endif
