#include <boost/test/unit_test.hpp>
#include <inplace/factory.hh>

#include <type_traits>

namespace {
  enum {
    NEITHER,
    COPY_ONLY,
    MOVE_ONLY,
    COPY_AND_MOVE
  };

  struct mixed_base {
    virtual ~mixed_base() { }
    virtual int val() const = 0;
  };

  struct mixed_copy_only : mixed_base {
    mixed_copy_only() = default;
    mixed_copy_only(mixed_copy_only const &) = default;
    mixed_copy_only(mixed_copy_only      &&) = delete;

    mixed_copy_only& operator=(mixed_copy_only const &) = default;
    mixed_copy_only& operator=(mixed_copy_only      &&) = delete;

    virtual int val() const { return COPY_ONLY; }
  };

  struct mixed_move_only : mixed_base {
    mixed_move_only() = default;
    mixed_move_only(mixed_move_only const &) = delete;
    mixed_move_only(mixed_move_only      &&) = default;

    mixed_move_only& operator=(mixed_move_only const &) = delete;
    mixed_move_only& operator=(mixed_move_only      &&) = default;

    virtual int val() const { return MOVE_ONLY; }
  };

  struct mixed_copy_and_move : mixed_base {
    virtual int val() const { return COPY_AND_MOVE; }
  };

  struct mixed_neither : mixed_base {
    mixed_neither() = default;
    mixed_neither(mixed_neither const &) = delete;
    mixed_neither(mixed_neither      &&) = delete;

    mixed_neither& operator=(mixed_neither const &) = delete;
    mixed_neither& operator=(mixed_neither      &&) = delete;

    virtual int val() const { return NEITHER; }
  };
}

BOOST_AUTO_TEST_SUITE(mixed_suite)

BOOST_AUTO_TEST_CASE(mixed_onlies) {
  typedef inplace::factory<mixed_base, mixed_copy_only, mixed_move_only> factory_t;

  BOOST_CHECK(!factory_t::has_copy_semantics);
  BOOST_CHECK( factory_t::has_move_semantics);

  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK( std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK( std::is_move_assignable   <factory_t>::value);

  factory_t fct;
  fct.construct<mixed_copy_only>();
  factory_t fct2(std::move(fct));

  BOOST_CHECK  (!fct );
  BOOST_REQUIRE( fct2);
  BOOST_CHECK_EQUAL(fct2->val(), COPY_ONLY);

  fct = std::move(fct2);

  BOOST_REQUIRE( fct );
  BOOST_CHECK  (!fct2);
  BOOST_CHECK_EQUAL(fct->val(), COPY_ONLY);

  fct = std::move(fct2);

  BOOST_CHECK(!fct );
  BOOST_CHECK(!fct2);

  fct.construct<mixed_move_only>();
  fct2 = std::move(fct);

  BOOST_CHECK  (!fct );
  BOOST_REQUIRE( fct2);
  BOOST_CHECK_EQUAL(fct2->val(), MOVE_ONLY);

  factory_t fct3(std::move(fct2));

  BOOST_CHECK  (!fct );
  BOOST_CHECK  (!fct2);
  BOOST_REQUIRE( fct3);
  BOOST_CHECK_EQUAL(fct3->val(), MOVE_ONLY);
}

BOOST_AUTO_TEST_CASE(mixed_noncopyables) {
  typedef inplace::factory<mixed_base,
                           mixed_move_only,
                           mixed_neither> factory_t;

  BOOST_CHECK(!factory_t::has_copy_semantics);
  BOOST_CHECK(!factory_t::has_move_semantics);

  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK(!std::is_move_assignable   <factory_t>::value);
}

BOOST_AUTO_TEST_CASE(mixed_nonmoveables) {
  typedef inplace::factory<mixed_base,
                           mixed_copy_only,
                           mixed_neither> factory_t;

  BOOST_CHECK(!factory_t::has_copy_semantics);
  BOOST_CHECK(!factory_t::has_move_semantics);

  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK(!std::is_move_assignable   <factory_t>::value);
}

BOOST_AUTO_TEST_CASE(mixed_all) {
  typedef inplace::factory<mixed_base,
                           mixed_copy_and_move,
                           mixed_copy_only,
                           mixed_move_only,
                           mixed_neither> factory_t;

  BOOST_CHECK(!factory_t::has_copy_semantics);
  BOOST_CHECK(!factory_t::has_move_semantics);

  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK(!std::is_move_assignable   <factory_t>::value);
}

BOOST_AUTO_TEST_CASE(mixed_neither_test) {
  typedef inplace::factory<mixed_base, mixed_neither> factory_t;

  BOOST_CHECK(!factory_t::has_copy_semantics);
  BOOST_CHECK(!factory_t::has_move_semantics);

  BOOST_CHECK(!std::is_copy_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_move_constructible<factory_t>::value);
  BOOST_CHECK(!std::is_copy_assignable   <factory_t>::value);
  BOOST_CHECK(!std::is_move_assignable   <factory_t>::value);
}

BOOST_AUTO_TEST_SUITE_END()
