#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace {
  enum {
    EXCEPT_NEVER      = 0,
    EXCEPT_ON_DEFAULT = 1,
    EXCEPT_ON_COPY    = 2,
    EXCEPT_ON_MOVE    = 4,
  };

  class ctor_exception : std::exception {
  public:
    ctor_exception(int flags) : flags_(flags) { }
    int flags() const { return flags_; }

  private:
    int flags_;
  };

  struct except_base {
    except_base() : p_(new int(123)) { }

    virtual ~except_base() { 
      BOOST_REQUIRE    (p_);
      BOOST_CHECK_EQUAL(*p_, 123);
    }
    virtual int val() const = 0;

  private:
    std::shared_ptr<int> p_;
  };

  template<unsigned exception_flags>
  struct except : except_base {
    except(              ) { if(exception_flags & EXCEPT_ON_DEFAULT) { throw ctor_exception(exception_flags); } }
    except(except const &) { if(exception_flags & EXCEPT_ON_COPY   ) { throw ctor_exception(exception_flags); } }
    except(except      &&) { if(exception_flags & EXCEPT_ON_MOVE   ) { throw ctor_exception(exception_flags); } }

    virtual int val() const { return exception_flags; }
  };

  typedef inplace::factory<except_base,
			   except<EXCEPT_NEVER>,
			   except<EXCEPT_ON_DEFAULT>,
			   except<EXCEPT_ON_COPY>,
			   except<EXCEPT_ON_COPY | EXCEPT_ON_DEFAULT>,
			   except<EXCEPT_ON_MOVE>,
			   except<EXCEPT_ON_MOVE | EXCEPT_ON_COPY>,
			   except<EXCEPT_ON_MOVE | EXCEPT_ON_COPY | EXCEPT_ON_DEFAULT>>
    factory_t;
}

BOOST_AUTO_TEST_SUITE(Exceptions)

BOOST_AUTO_TEST_CASE(ExceptionProperties) {
  BOOST_CHECK(factory_t::needs_copy_semantics);
  BOOST_CHECK(factory_t::needs_move_semantics);

  BOOST_CHECK(std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK(std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK(std::is_move_assignable   <factory_t>::value);
}

BOOST_AUTO_TEST_CASE(ExceptionDefault) {
  factory_t fct;

  BOOST_CHECK_THROW(fct.construct<except<EXCEPT_ON_DEFAULT>>(), ctor_exception);
  BOOST_CHECK(!fct);

  fct.construct<except<EXCEPT_NEVER>>();
  BOOST_CHECK(fct);

  BOOST_CHECK_THROW(fct.construct<except<EXCEPT_ON_DEFAULT>>(), ctor_exception);
  BOOST_CHECK(!fct);
}

BOOST_AUTO_TEST_CASE(ExceptionCopyCtor) {
  factory_t fct;
  fct.construct<except<EXCEPT_ON_COPY>>();

  BOOST_CHECK_THROW(factory_t fct2(fct), ctor_exception);
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), EXCEPT_ON_COPY);
}

BOOST_AUTO_TEST_CASE(ExceptionMoveCtor) {
  factory_t fct;
  fct.construct<except<EXCEPT_ON_MOVE>>();

  BOOST_CHECK_THROW(factory_t fct2(std::move(fct)), ctor_exception);
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), EXCEPT_ON_MOVE);
}

BOOST_AUTO_TEST_CASE(ExceptionCopyAssign) {
  factory_t fct, fct2;

  fct .construct<except<EXCEPT_ON_COPY>>();
  fct2.construct<except<EXCEPT_NEVER  >>();

  BOOST_CHECK_THROW(fct2 = fct, ctor_exception);
  BOOST_CHECK(!fct2);  
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), EXCEPT_ON_COPY);

  BOOST_CHECK_THROW(fct2 = fct, ctor_exception);
  BOOST_CHECK(!fct2);  
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), EXCEPT_ON_COPY);  
}

BOOST_AUTO_TEST_CASE(ExceptionMoveAssign) {
  factory_t fct, fct2;

  fct .construct<except<EXCEPT_ON_MOVE>>();
  fct2.construct<except<EXCEPT_NEVER  >>();

  BOOST_CHECK_THROW(fct2 = std::move(fct), ctor_exception);
  BOOST_CHECK(!fct2);  
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), EXCEPT_ON_MOVE);

  BOOST_CHECK_THROW(fct2 = std::move(fct), ctor_exception);
  BOOST_CHECK(!fct2);  
  BOOST_REQUIRE(fct);
  BOOST_CHECK_EQUAL(fct->val(), EXCEPT_ON_MOVE);  
}

BOOST_AUTO_TEST_SUITE_END()
