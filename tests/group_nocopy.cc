#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

#include <type_traits>

namespace {
  struct nocopy_base {
    nocopy_base() = default;
    nocopy_base(nocopy_base const &) = delete;
    nocopy_base(nocopy_base      &&) = default;
    virtual ~nocopy_base() { }

    virtual int val() const = 0;
  };

  template<int i>
  struct nocopy : nocopy_base {
    nocopy() = default;
    nocopy(nocopy const &) = delete;
    nocopy(nocopy      &&) = default;
    virtual int val() const { return i; }
  };

  class nocopy_x : public nocopy_base {
  public:
    nocopy_x(int x) : nocopy_base(), x_(x) { }
    nocopy_x(nocopy_x const &) = delete;
    nocopy_x(nocopy_x      &&) = default;
    virtual int val() const { return x_; }

  private:
    int x_;
  };

  typedef inplace::factory<nocopy_base,
                           nocopy<1>,
                           nocopy<2>,
                           nocopy_x> factory_t;
}

BOOST_AUTO_TEST_SUITE(nocopy_suite)

BOOST_AUTO_TEST_CASE(NoCopyProperties) {
  BOOST_CHECK(!factory_t::needs_copy_semantics);
  BOOST_CHECK( factory_t::needs_move_semantics);
}

BOOST_AUTO_TEST_CASE(NoCopyCopyCtor) {
  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
}

BOOST_AUTO_TEST_CASE(NoCopyMoveCtor) {
  BOOST_CHECK(std::is_move_constructible<factory_t>::value);

  factory_t fct;
  fct.construct<nocopy_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2(std::move(fct));

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_CASE(NoCopyCopyAssign) {
  BOOST_CHECK(!std::is_copy_assignable<factory_t>::value);
}

BOOST_AUTO_TEST_CASE(NoCopyMoveAssign) {
  BOOST_CHECK(std::is_move_assignable<factory_t>::value);

  factory_t fct;
  fct.construct<nocopy_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2;
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);

  fct2.construct<nocopy<1>>();

  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 1);

  fct.construct<nocopy_x>(10);
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_CASE(NoCopyConstructMove) {
  factory_t fct;
  nocopy_x orig(10);

  BOOST_CHECK_EQUAL(orig.val(), 10);

  fct.construct<nocopy_x>(std::move(orig));

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 10);
  BOOST_CHECK_EQUAL(orig.val(), 10);
}

BOOST_AUTO_TEST_SUITE_END()
