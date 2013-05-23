#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

namespace {
  struct nocopy_nomove_base {
    nocopy_nomove_base() = default;
    nocopy_nomove_base(nocopy_nomove_base const &) = delete;
    nocopy_nomove_base(nocopy_nomove_base      &&) = delete;
    virtual ~nocopy_nomove_base() { }

    virtual int val() const = 0;
  };

  template<int i>
  struct nocopy_nomove : nocopy_nomove_base {
    nocopy_nomove() = default;
    nocopy_nomove(nocopy_nomove const &) = delete;
    nocopy_nomove(nocopy_nomove      &&) = delete;
    virtual int val() const { return i; }
  };

  class nocopy_nomove_x : public nocopy_nomove_base {
  public:
    nocopy_nomove_x(int x) : nocopy_nomove_base(), x_(x) { }
    nocopy_nomove_x(nocopy_nomove_x const &) = delete;
    nocopy_nomove_x(nocopy_nomove_x      &&) = delete;
    virtual int val() const { return x_; }

  private:
    int x_;
  };

  typedef inplace::factory<nocopy_nomove_base,
                           nocopy_nomove<1>,
                           nocopy_nomove<2>,
                           nocopy_nomove_x> factory_t;
}

BOOST_AUTO_TEST_SUITE(nocopy_nomove_suite)

BOOST_AUTO_TEST_CASE(NoCopyNoMoveProperties) {
  BOOST_CHECK(!factory_t::has_copy_semantics);
  BOOST_CHECK(!factory_t::has_move_semantics);
}

BOOST_AUTO_TEST_CASE(NoCopyNoMoveCopyCtor) {
  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
}

BOOST_AUTO_TEST_CASE(NoCopyNoMoveMoveCtor) {
  BOOST_CHECK(!std::is_move_constructible<factory_t>::value);
}

BOOST_AUTO_TEST_CASE(NoCopyNoMoveCopyAssign) {
  BOOST_CHECK(!std::is_copy_assignable<factory_t>::value);
}

BOOST_AUTO_TEST_CASE(NoCopyNoMoveMoveAssign) {
  BOOST_CHECK(!std::is_move_assignable<factory_t>::value);
}

BOOST_AUTO_TEST_SUITE_END()
