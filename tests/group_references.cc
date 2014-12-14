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

  struct ref_impossible : std::exception { };
  struct ref_null       : std::exception { };

  class reference_holder : public reference_base {
  public:
    reference_holder(reference_holder const &) = default;
    reference_holder(reference_holder &&other) : ptr_(other.ptr_) {
      other.ptr_ = nullptr;
    }

    reference_holder(int &ref) : ptr_(&ref) { }
    reference_holder(int &&  ) {
      throw ref_impossible();
    }

    virtual int &val() const {
      if(!ptr_) {
        throw ref_null();
      }
      return *ptr_;
    }

  private:
    int *ptr_;
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
  BOOST_CHECK_THROW(fct.construct<reference_holder>(std::move(i)), ref_impossible);
  BOOST_CHECK(!fct);
}

BOOST_AUTO_TEST_CASE(ReferenceCopyCtor) {
  factory_t fct;
  int i = 10;

  fct.construct<reference_holder>(i);
  factory_t fct2(fct);

  BOOST_REQUIRE(fct);
  BOOST_REQUIRE(fct2);
  BOOST_CHECK_EQUAL(fct2->val(), fct->val());
  --i;
  BOOST_CHECK_EQUAL(fct ->val(), 9);
  BOOST_CHECK_EQUAL(fct2->val(), 9);
}

BOOST_AUTO_TEST_CASE(ReferenceMoveCtor) {
  factory_t fct;
  int i = 10;

  fct.construct<reference_holder>(i);
  factory_t fct2(std::move(fct));

  BOOST_CHECK  (!fct);
  BOOST_REQUIRE( fct2);
  BOOST_CHECK_EQUAL(fct2->val(), 10);
  --i;
  BOOST_CHECK_EQUAL(fct2->val(),  9);
}

BOOST_AUTO_TEST_CASE(ReferenceCopyAssign) {
  factory_t fct, fct2;
  int i = 10, j = 20;

  fct.construct<reference_holder>(i);
  fct2 = fct;

  BOOST_REQUIRE(fct );
  BOOST_REQUIRE(fct2);
  BOOST_CHECK_EQUAL(fct2->val(), fct->val());
  --i;
  BOOST_CHECK_EQUAL(fct ->val(), 9);
  BOOST_CHECK_EQUAL(fct2->val(), 9);

  fct2 = fct;

  BOOST_REQUIRE(fct );
  BOOST_REQUIRE(fct2);
  BOOST_CHECK_EQUAL(fct2->val(), fct->val());
  --i;
  BOOST_CHECK_EQUAL(fct ->val(), 8);
  BOOST_CHECK_EQUAL(fct2->val(), 8);

  fct.construct<reference_holder>(j);
  fct2 = fct;

  BOOST_REQUIRE(fct );
  BOOST_REQUIRE(fct2);
  BOOST_CHECK_EQUAL(fct2->val(), fct->val());
  --j;
  BOOST_CHECK_EQUAL(fct ->val(), 19);
  BOOST_CHECK_EQUAL(fct2->val(), 19);
}

BOOST_AUTO_TEST_CASE(ReferenceMoveAssign) {
  factory_t fct, fct2;
  int i = 10, j = 20;

  fct.construct<reference_holder>(i);
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct );
  BOOST_REQUIRE( fct2);
  --i;
  BOOST_CHECK_EQUAL(fct2->val(), 9);

  fct.construct<reference_holder>(i);
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct );
  BOOST_REQUIRE( fct2);
  --i;
  BOOST_CHECK_EQUAL(fct2->val(), 8);

  fct.construct<reference_holder>(j);
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct );
  BOOST_REQUIRE( fct2);
  --j;
  BOOST_CHECK_EQUAL(fct2->val(), 19);
}

BOOST_AUTO_TEST_SUITE_END()
