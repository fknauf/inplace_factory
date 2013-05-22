#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

namespace {
  struct plain_base {
    virtual ~plain_base() { }
    virtual int val() const = 0;
  };

  struct plain_child_1 : plain_base { virtual int val() const { return 1; } };
  struct plain_child_2 : plain_base { virtual int val() const { return 2; } };

  class plain_child_x : public plain_base {
  public:
    plain_child_x(int x) : x_(x) { }
    virtual int val() const { return x_; }

  private:
    int x_;
  };

  class plain_child_x_moveable : public plain_base {
  public:
    plain_child_x_moveable(int x) : x_(x) { }

    plain_child_x_moveable(plain_child_x_moveable const &) = default;
    plain_child_x_moveable(plain_child_x_moveable &&other)
      : x_(other.x_) {
      other.x_ = 0;
    }

    virtual int val() const { return x_; }

  private:
    int x_;
  };

  typedef inplace::factory<plain_base,
			   plain_child_1,
			   plain_child_2,
			   plain_child_x,
			   plain_child_x_moveable> factory_t;
}

BOOST_AUTO_TEST_SUITE(plain)

BOOST_AUTO_TEST_CASE(PlainProperties) {
  BOOST_CHECK(factory_t::has_copy_semantics);
  BOOST_CHECK(factory_t::has_move_semantics);
}

BOOST_AUTO_TEST_CASE(PlainDefaultCtor) {
  factory_t fct;

  BOOST_CHECK(!fct);
}

BOOST_AUTO_TEST_CASE(PlainCopyCtor) {
  factory_t fct;
  fct.construct<plain_child_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2(fct);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct ->val() == 10);
  BOOST_CHECK  (fct2->val() == 10);
}

BOOST_AUTO_TEST_CASE(PlainMoveCtor) {
  factory_t fct;
  fct.construct<plain_child_x>(10);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct->val() == 10);

  factory_t fct2(std::move(fct));

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK  (fct2->val() == 10);

  factory_t fct3(factory_t([](factory_t &f){ f.construct<plain_child_1>(); }));

  BOOST_REQUIRE(fct3);
  BOOST_CHECK  (fct3->val() == 1);

  factory_t fct4([]{ return factory_t([](factory_t &f) { f.construct<plain_child_1>(); }); }());

  BOOST_REQUIRE(fct3);
  BOOST_CHECK  (fct3->val() == 1);
}

BOOST_AUTO_TEST_CASE(PlainLambdaCtor) {
  factory_t fct([](factory_t &f) { f.construct<plain_child_1>(); });

  BOOST_REQUIRE(fct);
  BOOST_CHECK(fct->val() == 1);

  factory_t fct2([](factory_t &f, int n) { f.construct<plain_child_x>(n); }, 10);

  BOOST_REQUIRE(fct2);
  BOOST_CHECK(fct2->val() == 10);

  factory_t fct3([](factory_t &f, int n) {
      if(n < 10) {
	f.construct<plain_child_1>();
      } else {
	f.construct<plain_child_x>(n);
      }
    }, 5);

  BOOST_REQUIRE(fct3);
  BOOST_CHECK(fct3->val() == 1);
}

BOOST_AUTO_TEST_CASE(PlainConstruct) {
  factory_t fct;

  fct.construct<plain_child_1>();

  BOOST_REQUIRE(fct.is_initialized());
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 1);
  BOOST_CHECK_EQUAL(fct.get().val(), fct->val());

  fct.construct<plain_child_2>();

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 2);
  BOOST_CHECK_EQUAL(fct.get().val(), fct->val());

  fct.clear();

  BOOST_CHECK(!fct.is_initialized());
  BOOST_CHECK(!fct);

  fct.construct<plain_child_x>(3);

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 3);
  BOOST_CHECK_EQUAL(fct.get().val(), fct->val());
}

BOOST_AUTO_TEST_CASE(PlainConstructCopy) {
  factory_t fct;
  plain_child_x orig(10);

  fct.construct<plain_child_x>(orig);

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 10);
}

BOOST_AUTO_TEST_CASE(PlainConstructMove) {
  factory_t fct;
  plain_child_x_moveable orig(10);

  BOOST_CHECK_EQUAL(orig.val(), 10);

  fct.construct<plain_child_x_moveable>(std::move(orig));

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 10);
  BOOST_CHECK_EQUAL(orig.val(), 0);
}

BOOST_AUTO_TEST_CASE(PlainClear) {
  factory_t fct;
  fct.construct<plain_child_1>();

  BOOST_CHECK(fct);

  fct.clear();

  BOOST_CHECK(!fct);
}

BOOST_AUTO_TEST_CASE(PlainInspection) {
  factory_t fct;

  BOOST_CHECK(!static_cast<bool>(fct));
  BOOST_CHECK(!fct);
  BOOST_CHECK(fct.get_ptr() == nullptr);
  BOOST_CHECK(!fct.is_initialized());

  fct.construct<plain_child_1>();

  BOOST_CHECK(static_cast<bool>(fct));
  BOOST_CHECK(!!fct);
  BOOST_CHECK(fct.get_ptr() != nullptr);
  BOOST_CHECK(fct.is_initialized());

  BOOST_CHECK_EQUAL(fct->val(), 1);
  BOOST_CHECK_EQUAL(fct->val(), fct.get    ().val());
  BOOST_CHECK_EQUAL(fct->val(), fct.get_ptr()->val());
  BOOST_CHECK_EQUAL(fct->val(), (*fct).val());
}

BOOST_AUTO_TEST_SUITE_END()
