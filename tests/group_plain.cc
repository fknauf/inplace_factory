#include <boost/test/unit_test.hpp>

#include <inplace/factory.hh>
#include <typeinfo>

namespace {
  struct plain_base {
    virtual int id() const = 0;
    virtual ~plain_base() { }
  };

  struct plain_child_1 : plain_base { virtual int id() const { return 1; } };
  struct plain_child_2 : plain_base { virtual int id() const { return 2; } };

  class plain_child_x : public plain_base {
  public:
    plain_child_x(int x) : x_(x) { }
    virtual int id() const { return x_; }

  private:
    int x_;
  };

  typedef inplace::factory<plain_base,
			   plain_child_1,
			   plain_child_2,
			   plain_child_x> factory_t;
}

BOOST_AUTO_TEST_SUITE(plain)

BOOST_AUTO_TEST_CASE( PlainConstruct ) {
  factory_t fct;

  BOOST_CHECK(!fct.is_initialized());
  BOOST_CHECK(!fct);
  BOOST_CHECK(fct.get_ptr() == nullptr);

  fct.construct<plain_child_1>();

  BOOST_REQUIRE(fct.is_initialized());
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->id(), 1);
  BOOST_CHECK_EQUAL(fct.get().id(), fct->id());

  fct.construct<plain_child_2>();

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->id(), 2);
  BOOST_CHECK_EQUAL(fct.get().id(), fct->id());

  fct.clear();

  BOOST_CHECK(!fct.is_initialized());
  BOOST_CHECK(!fct);
  BOOST_CHECK(fct.get_ptr() == nullptr);

  fct.construct<plain_child_x>(3);

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->id(), 3);
  BOOST_CHECK_EQUAL(fct.get().id(), fct->id());
}

BOOST_AUTO_TEST_SUITE_END()
