#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

#include <memory>
#include <type_traits>
#include <typeinfo>

namespace {
  enum {
    FRONT,
    BACK,
    SANDWICH,

    MADE_WITH_DEFAULT,
    MADE_WITH_COPY,
    MADE_WITH_MOVE
  };

  struct multi_base {
    multi_base()                        : made_with_(MADE_WITH_DEFAULT), p_(new int(123)) { }
    multi_base(multi_base const &other) : made_with_(MADE_WITH_COPY   ), p_(other.p_    ) { }
    multi_base(multi_base      &&other) : made_with_(MADE_WITH_MOVE   ), p_(other.p_    ) { }

    multi_base &operator=(multi_base const &) { made_with_ = -1; return *this; }
    multi_base &operator=(multi_base      &&) { made_with_ = -1; return *this; }

    virtual ~multi_base() {
      BOOST_REQUIRE_EQUAL(test_pattern_, 0xf001f001);
      BOOST_REQUIRE(p_ && *p_ == 123);
    }

    virtual int val() const = 0;

    int made_with() const {
      return made_with_;
    }

  private:
    int made_with_;
    int test_pattern_ = 0xf001f001;
    std::shared_ptr<int> p_;
  };

  struct filler1 { int padding[10]; };
  struct filler2 { int padding[20]; };
  struct filler3 { int padding[30]; };

  struct multi_front    :          multi_base, filler1 { virtual int val() const { return FRONT   ; } };
  struct multi_back     : filler1, multi_base          { virtual int val() const { return BACK    ; } };
  struct multi_sandwich : filler2, multi_base, filler3 { virtual int val() const { return SANDWICH; } };

  typedef inplace::factory<multi_base,
                           multi_front,
                           multi_back,
                           multi_sandwich> factory_t;
}

BOOST_AUTO_TEST_SUITE(multi_suite)

BOOST_AUTO_TEST_CASE(MultiProperties) {
  BOOST_CHECK(factory_t::needs_copy_semantics);
  BOOST_CHECK(factory_t::needs_move_semantics);

  BOOST_CHECK(std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK(std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK(std::is_move_assignable   <factory_t>::value);
}

BOOST_AUTO_TEST_CASE(MultiConstruct) {
  factory_t fct;
  fct.construct<multi_front>();

  BOOST_REQUIRE    (fct);
  BOOST_CHECK_EQUAL(fct->val(), FRONT);

  fct.construct<multi_back>();

  BOOST_REQUIRE    (fct);
  BOOST_CHECK_EQUAL(fct->val(), BACK);

  fct.construct<multi_sandwich>();

  BOOST_REQUIRE    (fct);
  BOOST_CHECK_EQUAL(fct->val(), SANDWICH);
}

BOOST_AUTO_TEST_CASE(MultiCopyCtor) {
  factory_t fct;
  fct.construct<multi_sandwich>();

  factory_t fct2(fct);

  BOOST_REQUIRE(fct );
  BOOST_REQUIRE(fct2);
  BOOST_CHECK_EQUAL(fct ->val(), SANDWICH);
  BOOST_CHECK_EQUAL(fct2->val(), SANDWICH);
}

BOOST_AUTO_TEST_CASE(MultiMoveCtor) {
  factory_t fct;
  fct.construct<multi_sandwich>();

  factory_t fct2(std::move(fct));

  BOOST_CHECK  (!fct );
  BOOST_REQUIRE( fct2);
  BOOST_CHECK_EQUAL(fct2->val(), SANDWICH);
}

BOOST_AUTO_TEST_CASE(MultiCopyAssign) {
  factory_t fct, fct2, fct3;

  fct.construct <multi_sandwich>();
  fct2.construct<multi_front   >();

  fct3 = fct2;
  fct2 = fct ;

  BOOST_REQUIRE(fct );
  BOOST_REQUIRE(fct2);
  BOOST_REQUIRE(fct3);
  BOOST_CHECK_EQUAL(fct ->val(), SANDWICH);
  BOOST_CHECK_EQUAL(fct2->val(), SANDWICH);
  BOOST_CHECK_EQUAL(fct3->val(), FRONT   );
}

BOOST_AUTO_TEST_CASE(MultiMoveAssign) {
  factory_t fct, fct2, fct3;

  fct.construct <multi_sandwich>();
  fct2.construct<multi_front   >();

  fct2 = std::move(fct );
  fct3 = std::move(fct2);

  BOOST_REQUIRE(!fct );
  BOOST_REQUIRE(!fct2);
  BOOST_REQUIRE( fct3);
  BOOST_CHECK_EQUAL(fct3->val(), SANDWICH);
}

BOOST_AUTO_TEST_SUITE_END()
