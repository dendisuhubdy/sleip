#include <sleip/dynamic_array.hpp>

#include <boost/core/default_allocator.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

#include <algorithm>
#include <array>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <boost/core/lightweight_test.hpp>

#ifdef BOOST_NO_EXCEPTIONS

#include <iostream>
#include <exception>

namespace boost
{
void
throw_exception(std::exception const& e)
{
  std::cerr << "Exception generated in noexcept code\nError: " << e.what() << "\n\n";
  std::terminate();
}
} // namespace boost
#endif

namespace pmr = boost::container::pmr;

void
test_default_constructible()
{
  static_assert(noexcept(sleip::dynamic_array<int>()),
                "Default allocator + default constructor must be noexcept");

  sleip::dynamic_array<int> buf;

  BOOST_TEST_EQ(buf.size(), 0);
  BOOST_TEST_EQ(std::distance(buf.begin(), buf.end()), 0);
  BOOST_TEST_EQ(buf.data(), nullptr);
  BOOST_TEST(buf.get_allocator() == std::allocator<int>{});

  sleip::dynamic_array<int, pmr::polymorphic_allocator<int>> pmr_buf;

  BOOST_TEST_EQ(pmr_buf.size(), 0);
  BOOST_TEST_EQ(std::distance(pmr_buf.begin(), pmr_buf.end()), 0);
  BOOST_TEST_EQ(pmr_buf.data(), nullptr);
  BOOST_TEST(pmr_buf.get_allocator() == pmr::polymorphic_allocator<int>{});
}

void
test_allocator_constructor()
{
  auto alloc = boost::default_allocator<int>();

  static_assert(std::is_same_v<typename decltype(alloc)::value_type,
                               typename sleip::dynamic_array<int>::value_type>);

  sleip::dynamic_array<int, boost::default_allocator<int>> buf(alloc);

  BOOST_TEST(buf.get_allocator() == alloc);

  BOOST_TEST_EQ(buf.size(), 0);
  BOOST_TEST_EQ(std::distance(buf.begin(), buf.end()), 0);
  BOOST_TEST_EQ(buf.data(), nullptr);
}

void
test_value_constructible()
{
  auto const count = std::size_t{24};
  auto const value = -1;

  sleip::dynamic_array<int> buf(count, value);

  BOOST_TEST_EQ(buf.size(), count);
  BOOST_TEST(std::all_of(buf.begin(), buf.end(), [=](auto const v) { return v == value; }));
}

#ifdef BOOST_NO_EXCEPTIONS
void
test_value_constructible_throwing()
{
}
#else
void
test_value_constructible_throwing()
{
  struct throwing
  {
    std::size_t&         idx;
    std::array<char, 6>& c_out;
    std::array<char, 6>& d_out;

    throwing() = delete;
    throwing(throwing const& other)
      : idx{other.idx}
      , c_out(other.c_out)
      , d_out(other.d_out)
    {
      if ((idx + 1) == 6) { throw 42; }

      c_out[idx] = static_cast<char>('a' + idx);
      ++idx;
    }

    throwing(throwing&&) = delete;

    ~throwing()
    {
      if (idx == std::size_t(-1)) { return; }
      d_out[5 - idx] = static_cast<char>('a' + idx - 1);

      --idx;
    }

    throwing(std::size_t& idx_, std::array<char, 6>& c_out_, std::array<char, 6>& d_out_)
      : idx(idx_)
      , c_out(c_out_)
      , d_out(d_out_)
    {
    }
  };

  auto constructor_out = std::array<char, 6>{};
  auto destructor_out  = std::array<char, 6>{};
  auto idx             = std::size_t{0};

  auto const value = throwing(idx, constructor_out, destructor_out);

  auto const expected_c_out = std::string_view("abcde");
  auto const expected_d_out = std::string_view("edcba");

  BOOST_TEST_THROWS((sleip::dynamic_array<throwing>(6, value)), int);

  BOOST_TEST_ALL_EQ(constructor_out.begin(), constructor_out.end() - 1, expected_c_out.begin(),
                    expected_c_out.end());

  BOOST_TEST_ALL_EQ(destructor_out.begin(), destructor_out.end() - 1, expected_d_out.begin(),
                    expected_d_out.end());
}
#endif

void
test_size_constructible()
{
  auto const count = std::size_t{24};

  sleip::dynamic_array<int> buf(count);

  BOOST_TEST_EQ(buf.size(), 24);
  BOOST_TEST(std::all_of(buf.begin(), buf.end(), [](auto const v) { return v == int{}; }));
}

#ifdef BOOST_NO_EXCEPTIONS
void
test_size_constructible_throwing()
{
}
#else
void
test_size_constructible_throwing()
{
  struct foo_throwing
  {
    foo_throwing() { throw 42; }
    foo_throwing(foo_throwing const&) = delete;
    foo_throwing(foo_throwing&&)      = delete;
  };

  auto const count = std::size_t{24};

  BOOST_TEST_THROWS((sleip::dynamic_array<foo_throwing>(count)), int);
}
#endif

void
test_iterator_constructible()
{
  auto const nums = std::vector{1, 2, 3, 4, 5};

  sleip::dynamic_array<int> buf(nums.begin(), nums.end());

  BOOST_TEST_EQ(buf.size(), nums.size());
  BOOST_TEST_ALL_EQ(buf.begin(), buf.end(), nums.begin(), nums.end());
}

void
test_copy_constructible()
{
  auto const nums = std::vector{1, 2, 3, 4, 5};

  sleip::dynamic_array<int> const buf(nums.begin(), nums.end());
  sleip::dynamic_array<int> const buf2(buf);

  BOOST_TEST_EQ(buf2.size(), buf.size());
  BOOST_TEST_ALL_EQ(buf2.begin(), buf2.end(), buf.begin(), buf.end());
}

void
test_copy_constructible_allocator()
{
  auto const nums = std::vector{1, 2, 3, 4, 5};

  sleip::dynamic_array<int, boost::default_allocator<int>> const buf(
    nums.begin(), nums.end(), boost::default_allocator<int>());

  sleip::dynamic_array<int, boost::default_allocator<int>> const buf2(
    buf, boost::default_allocator<int>());

  BOOST_TEST_EQ(buf2.size(), buf.size());
  BOOST_TEST_ALL_EQ(buf2.begin(), buf2.end(), buf.begin(), buf.end());
}

void
test_move_constructible()
{
  auto const nums = std::vector{1, 2, 3, 4, 5};

  sleip::dynamic_array<int>       buf(nums.begin(), nums.end());
  sleip::dynamic_array<int> const buf2(std::move(buf));

  static_assert(noexcept(sleip::dynamic_array<int>(std::move(buf))));

  BOOST_TEST_EQ(buf.size(), 0);
  BOOST_TEST_EQ(buf2.size(), nums.size());
  BOOST_TEST_ALL_EQ(buf2.begin(), buf2.end(), nums.begin(), nums.end());

  // BOOST_TEST(buf.empty());
}

void
test_move_constructible_allocator()
{
  auto const nums = std::vector{1, 2, 3, 4, 5};

  auto alloc = boost::default_allocator<int>();

  sleip::dynamic_array<int, boost::default_allocator<int>> buf(nums.begin(), nums.end(), alloc);
  sleip::dynamic_array<int, boost::default_allocator<int>> buf2(std::move(buf), alloc);

  BOOST_TEST(buf.get_allocator() == buf2.get_allocator());
  BOOST_TEST_EQ(buf.size(), 0);
  BOOST_TEST_EQ(buf2.size(), nums.size());
  BOOST_TEST_ALL_EQ(buf2.begin(), buf2.end(), nums.begin(), nums.end());
}

void
test_initializer_list_constructible()
{
  auto buf = sleip::dynamic_array<int>{1, 2, 3, 4, 5};

  auto const nums = std::initializer_list<int>{1, 2, 3, 4, 5};

  BOOST_TEST_EQ(buf.size(), 5);
  BOOST_TEST_ALL_EQ(buf.begin(), buf.end(), nums.begin(), nums.end());
}

void
test_range_constructible()
{
  using std::begin;
  using std::end;

  static_assert(sleip::detail::is_range_v<int[3]>);
  static_assert(!sleip::detail::is_range_v<int>);

  {
    auto a = std::vector<int>{1, 2, 3, 4, 5};
    auto b = sleip::dynamic_array<int>(a);

    BOOST_TEST_ALL_EQ(a.begin(), a.end(), b.begin(), b.end());
  }

  {
    auto a = std::list<int>{1, 2, 3, 4, 5};
    auto b = sleip::dynamic_array<int>(a);

    BOOST_TEST_ALL_EQ(b.begin(), b.end(), a.begin(), a.end());
  }

  {
    int a[5] = {1, 2, 3, 4, 5};

    auto b = sleip::dynamic_array<int>(a);

    BOOST_TEST_ALL_EQ(b.begin(), b.end(), begin(a), end(a));
  }
}

int
main()
{
  test_default_constructible();
  test_allocator_constructor();
  test_value_constructible();
  test_value_constructible_throwing();
  test_size_constructible();
  test_size_constructible_throwing();
  test_iterator_constructible();
  test_copy_constructible();
  test_copy_constructible_allocator();
  test_move_constructible();
  test_move_constructible_allocator();
  test_initializer_list_constructible();
  test_range_constructible();

  return boost::report_errors();
}
