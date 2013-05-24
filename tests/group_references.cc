#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace {
  struct reference_base {
    virtual ~reference_base() { }
    virtual int &val() const = 0;
  };

  int default_ref;
  struct ref_impossible : std::exception { };

  class reference_holder : public reference_base {
  public:
    reference_holder(int &ref) : ref_(ref) { }
    reference_holder(int &&  ) : ref_(default_ref) {
      throw ref_impossible();
    }

    virtual int &val() const { return ref_; }

  private:
    int &ref_;
  };

  typedef inplace::factory<reference_base,
			   reference_holder>
  factory_t;
}

BOOST_AUTO_TEST_SUITE(References)

BOOST_AUTO_TEST_CASE(ReferenceConstruct) {
  factory_t fct;
  int i = 10;

  fct.construct<reference_holder>(i);

  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), 10);
  --i;
  BOOST_CHECK_EQUAL(fct->val(),  9);

  BOOST_CHECK_THROW(fct.construct<reference_holder>(2), ref_impossible);
}

BOOST_AUTO_TEST_SUITE_END()
