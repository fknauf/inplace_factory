#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

namespace {
  struct nomove_base {
    nomove_base() = default;
    nomove_base(nomove_base const &) = default;
    nomove_base(nomove_base      &&) = delete;
    virtual ~nomove_base() { }

    virtual int val() const = 0;
  };

  template<int i>
  struct nomove : nomove_base {
    nomove() = default;
    nomove(nomove const &) = default;
    nomove(nomove      &&) = delete;
    virtual int val() const { return i; }
  };

  class nomove_x : public nomove_base {
  public:
    nomove_x(int x) : nomove_base(), x_(x) { }
    nomove_x(nomove_x const &) = default;
    nomove_x(nomove_x      &&) = delete;
    virtual int val() const { return x_; }

  private:
    int x_;
  };

  typedef inplace::factory<nomove_base,
                           nomove<1>,
                           nomove<2>,
                           nomove_x> factory_t;
}

BOOST_AUTO_TEST_SUITE(nomove_suite)

BOOST_AUTO_TEST_CASE(NoMoveProperties) {
  BOOST_CHECK( factory_t::needs_copy_semantics);
  BOOST_CHECK(!factory_t::needs_move_semantics);
}

BOOST_AUTO_TEST_CASE(NoMoveCopyCtor) {
  BOOST_CHECK(std::is_copy_constructible<factory_t>::value);

  factory_t fct;
  fct.construct<nomove_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2(fct);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct ->val() == 10);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_CASE(NoMoveMoveCtor) {
  BOOST_CHECK(std::is_move_constructible<factory_t>::value);

  factory_t fct;
  fct.construct<nomove_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2(std::move(fct));

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_CASE(NoMoveConstructCopy) {
  factory_t fct;
  nomove_x orig(10);

  fct.construct<nomove_x>(orig);

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 10);
  BOOST_CHECK_EQUAL(orig.val(), 10);
}

BOOST_AUTO_TEST_CASE(NoMoveConstructMove) {
  factory_t fct;
  nomove_x orig(10);

  BOOST_CHECK_EQUAL(orig.val(), 10);

  fct.construct<nomove_x>(std::move(orig));

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 10);
  BOOST_CHECK_EQUAL(orig.val(), 10);
}

BOOST_AUTO_TEST_CASE(NoMoveCopyAssign) {
  BOOST_CHECK(std::is_copy_assignable<factory_t>::value);

  factory_t fct;
  fct.construct<nomove_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2;

  fct2 = fct;

  BOOST_REQUIRE(fct);
  BOOST_CHECK  (fct ->val() == 10);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);

  fct2.construct<nomove<1>>();

  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val());

  fct2 = fct;

  BOOST_REQUIRE(fct);
  BOOST_CHECK  (fct ->val() == 10);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_CASE(NoMoveMoveAssign) {
  BOOST_CHECK(std::is_move_assignable<factory_t>::value);

  factory_t fct;
  fct.construct<nomove_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2;

  fct2 = std::move(fct);

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);

  fct2.construct<nomove<1>>();

  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val());

  fct.construct<nomove_x>(10);
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_SUITE_END()
