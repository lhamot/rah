//
// Copyright (c) 2016 Loïc HAMOT
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "rah.hpp"

#ifdef MSVC
#pragma warning(push, 0)
#endif
#include <iostream>
#include <vector>
#include <map>
#include <forward_list>
#ifdef MSVC
#pragma warning(pop)
#endif

template<typename A, typename B, typename C, typename D>
bool operator == (std::tuple<A, B> a, std::pair<D, C> b)
{
	return std::get<0>(a) == std::get<0>(b) && std::get<1>(a) == std::get<1>(b);
}

template<typename A, typename B, typename C, typename D>
bool operator == (std::pair<A, B> a, std::tuple<D, C> b)
{
	return std::get<0>(a) == std::get<0>(b) && std::get<1>(a) == std::get<1>(b);
}

auto PairEqual = [](auto ab) {return std::get<0>(ab) == std::get<1>(ab); };

#define CHECK(CONDITION) \
std::cout << "CHECK : " << #CONDITION << std::endl; \
if(CONDITION) \
	std::cout << "OK" << std::endl; \
else \
	{std::cout << "NOT OK" << std::endl; abort();}

#define EQUAL_RANGE(RANGE, IL) \
std::cout << "CHECK : " << #RANGE << " = " << #IL << std::endl; \
if(zip(RANGE, IL) | all_of(PairEqual)) \
	std::cout << "OK" << std::endl; \
else \
	{std::cout << "NOT OK" << std::endl; abort();}

template<typename T>
using il = std::initializer_list<T>;

auto print_elt = [](auto&& elt)
{
	(std::cout << std::forward<decltype(elt)>(elt)) << " ";
};

template<typename... Args>
std::ostream& operator << (std::ostream& os, std::tuple<Args...> tup)
{
	::rah::view::details::for_each(tup, print_elt);
	return os;
}

namespace test
{
	template< class T >
    constexpr bool is_reference_v = std::is_reference<T>::value;
	template< class T >
    constexpr bool is_rvalue_reference_v = std::is_rvalue_reference<T>::value;
}

template<typename T>
struct WhatIsIt;

int main()
{
	using namespace rah;
	using namespace rah::view;
	using namespace std;

	struct Elt
	{
		int member;
		bool operator==(Elt elt) const
		{
			return member == elt.member;
		}
	};

	// *********************************** views **************************************************

	// Test iota
	EQUAL_RANGE(iota(0, 4), il<int>({ 0, 1, 2, 3 }));
	EQUAL_RANGE(iota(10, 20, 2), il<int>({ 10, 12, 14, 16, 18 }));

	// Test generate
	int y = 1;
	EQUAL_RANGE(
		generate([&y]() { auto prev = y; y *= 2; return prev; }) | slice(0, 4), 
		(il<int>{1, 2, 4, 8})
	);
	y = 1;
	EQUAL_RANGE(
		generate_n([&y]() { auto prev = y; y *= 2; return prev; }, 4), 
		(il<int>{1, 2, 4, 8})
	);

	// Test all
	EQUAL_RANGE((il<int>{0, 1, 2, 3} | all()), (il<int>{ 0, 1, 2, 3 }));

	// Test transform
	EQUAL_RANGE(iota(0, 4) | transform([](auto a) {return a * 2; }), il<int>({ 0, 2, 4, 6 }));
	EQUAL_RANGE(transform(iota(0, 4), [](auto a) {return a * 2; }), il<int>({ 0, 2, 4, 6 }));

	// Test slice
	EQUAL_RANGE(iota(0, 100) | slice(10, 15), il<int>({ 10, 11, 12, 13, 14 }));
	EQUAL_RANGE(slice(iota(0, 100), 10, 15), il<int>({ 10, 11, 12, 13, 14 }));

	// Test stride
	EQUAL_RANGE(iota(0, 100) | stride(20), il<int>({ 0, 20, 40, 60, 80 }));
	EQUAL_RANGE(stride(iota(0, 100), 20), il<int>({ 0, 20, 40, 60, 80 }));

	// Test retro
	EQUAL_RANGE(iota(0, 4) | retro(), il<int>({ 3, 2, 1, 0 }));
	EQUAL_RANGE(retro(iota(0, 4)), il<int>({ 3, 2, 1, 0 }));

	// Test zip
	using tIDC = std::tuple<int, double, char>;
	EQUAL_RANGE(
		zip(il<int>{ 1, 2, 3, 4 }, il<double>{ 2.5, 4.5, 6.5, 8.5 }, il<char>{ 'a', 'b', 'c', 'd' }),
		(il<tuple<int, double, char>>{ 
			tIDC{ 1, 2.5, 'a' }, tIDC{ 2, 4.5, 'b' }, tIDC{ 3, 6.5, 'c' }, tIDC{ 4, 8.5, 'd' } })
	);

	// Test chuck
	{
		std::vector<int> vec_01234{ 0, 1, 2, 3, 4 };
		auto to_test = vec_01234 | chunk(2);
		std::vector<vector<int>> ref({ {0, 1}, { 2, 3 }, { 4 } });
		CHECK(size(to_test) == size(ref));
		for (auto elt : zip(to_test, ref))
		{
			CHECK(size(std::get<0>(elt)) == size(std::get<1>(elt)));
			CHECK(equal(std::get<0>(elt), std::get<1>(elt)));
		}
	}

	// Test filter
	EQUAL_RANGE(iota(0, 8) | filter([](auto a) {return a % 2 == 0; }), il<int>({ 0, 2, 4, 6 }));
	EQUAL_RANGE(filter(iota(0, 8), [](auto a) {return a % 2 == 0; }), il<int>({ 0, 2, 4, 6 }));

	// Test join
	EQUAL_RANGE((il<int>{ 0, 1, 2, 3 } | join(il<int>{ 4, 5, 6 })), (il<int>{0, 1, 2, 3, 4, 5, 6}));
	EQUAL_RANGE((join(il<int>{ 0, 1, 2, 3 }, il<int>{ 4, 5, 6 })), (il<int>{0, 1, 2, 3, 4, 5, 6}));

	// Test enumerate
	using tUII = std::tuple<unsigned int, int>;
	EQUAL_RANGE(
		iota(4, 8) | enumerate(), 
		(il<tuple<unsigned int, int>>{ tUII{ 0, 4 }, tUII{ 1, 5 }, tUII{ 2, 6 }, tUII{ 3, 7 } })
	);
	EQUAL_RANGE(
		enumerate(iota(4, 8)), 
		(il<tuple<unsigned int, int>>{ tUII{ 0, 4 }, tUII{ 1, 5 }, tUII{ 2, 6 }, tUII{ 3, 7 } })
	);

	// Test map_value
	EQUAL_RANGE(
		(std::map<int, double>{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 }} | map_value()), 
		(il<double>{ 1.5, 2.5, 3.5, 4.5 })
	);

	// Test map_key
	EQUAL_RANGE(
		(std::map<int, double>{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 }} | map_key()), 
		(il<int>{ 1, 2, 3, 4 })
	);

	// ************************************ eager algos *******************************************

	// Test transform
	{
		std::vector<int> vec1{ 0, 1, 2, 3 };
		std::vector<int> vec2{ 4, 3, 2, 1 };
		std::vector<int> vec3{ 0, 0, 0, 0 };
		rah::transform(vec1, vec3, [](int a) {return a + 1; });
		EQUAL_RANGE(vec3, il<int>({ 1, 2, 3, 4 }));
		rah::transform(vec1, vec2, vec3, [](int a, int b) {return a + b; });
		EQUAL_RANGE(vec3, il<int>({ 4, 4, 4, 4 }));
	}

	// Test reduce
	CHECK((iota(0, 0) | reduce(0, [](auto a, auto b) {return a + b; })) == 0);
	CHECK((iota(1, 5) | reduce(0, [](auto a, auto b) {return a + b; })) == 10);
	CHECK(reduce(iota(1, 5), 0, [](auto a, auto b) {return a + b; }) == 10);

	// Test any_of
	CHECK(any_of(il<int>{3, 0, 1, 3, 4, 6}, [](auto a) {return a == 3; }));
	CHECK((il<int>{3, 0, 1, 3, 4, 6} | any_of([](auto a) {return a == 3; })));
	CHECK((il<int>{2, 0, 1, 3, 4, 6} | any_of([](auto a) {return a == 3; })));
	CHECK((il<int>{2, 0, 1, 2, 4, 6} | any_of([](auto a) {return a == 3; })) == false);

	// Test all_of
	CHECK(all_of(il<int>{ 4, 4, 4, 4 }, [](auto a) {return a == 4; }));
	CHECK(all_of(il<int>{ 4, 4, 3, 4 }, [](auto a) {return a == 4; }) == false);
	CHECK((il<int>{ 4, 4, 4, 4 } | all_of([](auto a) {return a == 4; })));
	CHECK((il<int>{ 4, 4, 3, 4 } | all_of([](auto a) {return a == 4; })) == false);

	// Test none_of
	CHECK((none_of(il<int>{7, 8, 9, 10}, [](auto a) {return a == 11; })));
	CHECK((il<int>{7, 8, 9, 10} | none_of([](auto a) {return a == 11; })));
	CHECK((il<int>{7, 8, 9, 10, 11} | none_of([](auto a) {return a == 11; })) == false);

	// Test count
	CHECK((il<int>{ 4, 4, 4, 3 } | count(4)) == 3);
	CHECK(count(il<int>{ 4, 4, 4, 3 }, 3) == 1);

	// Test count_if
	CHECK((il<int>{ 4, 4, 4, 3 } | count_if([](auto a) {return a == 3; })) == 1);
	CHECK(count_if(il<int>{ 4, 4, 4, 3 }, [](auto a) {return a == 4; }) == 3);

	// Test foreach
	{
		std::vector<int> testFE{ 4, 4, 4, 4 };
		for_each(testFE, [](auto& value) {return ++value; });
		EQUAL_RANGE(testFE, il<int>({ 5, 5, 5, 5 }));
		testFE | for_each([](auto& value) {return ++value; });
		EQUAL_RANGE(testFE, il<int>({ 6, 6, 6, 6 }));
	}

	// Test to_container
	{
		EQUAL_RANGE(iota(4, 8) | to_container<std::vector<int>>(), (il<int>{ 4, 5, 6, 7 }));

		auto toPair = [](auto ab) {return std::make_pair(std::get<0>(ab), std::get<1>(ab)); };
		auto&& map_4a_5b_6c_7d = 
			zip(iota(4, 8), iota('a', 'e')) | transform(toPair) | to_container<std::map<int, char>>();
		EQUAL_RANGE(
			map_4a_5b_6c_7d, 
			(il<std::pair<int const, char>>{ {4, 'a'}, { 5, 'b' }, { 6, 'c' }, { 7, 'd' } })
		);
	}

	// Test mismatch
	{
		auto a = { 1, 2, 3, 4 };
		auto b = { 1, 2, 42, 42 };
		auto r1_r2 = mismatch(a, b);
		EQUAL_RANGE(std::get<0>(r1_r2), il<int>({ 3, 4 }));
		EQUAL_RANGE(std::get<1>(r1_r2), il<int>({ 42, 42 }));
	}

	// Test find
	{
		EQUAL_RANGE(find(il<int>{ 1, 2, 3, 4 }, 3), il<int>({3, 4}));
		EQUAL_RANGE((il<int>{ 1, 2, 3, 4 } | find(3)), il<int>({ 3, 4 }));
		EQUAL_RANGE(find_if(il<int>{ 1, 2, 3, 4 }, [](int i) {return i == 3; }), il<int>({ 3, 4 }));
		EQUAL_RANGE((il<int>{ 1, 2, 3, 4 } | find_if([](int i) {return i == 3; })), il<int>({ 3, 4 }));
		EQUAL_RANGE(find_if_not(il<int>{ 1, 2, 3, 4 }, [](int i) {return i < 3; }), il<int>({ 3, 4 }));
		EQUAL_RANGE((il<int>{ 1, 2, 3, 4 } | find_if_not([](int i) {return i < 3; })), il<int>({ 3, 4 }));
	}

	// Test return reference

	{
		std::vector<Elt> vec = { {0}, { 1 }, { 2 }, { 3 }, { 4 } };
		auto r = vec | all();
		for (auto iter = std::begin(r), end = std::end(r); iter != end; ++iter)
		{
			iter->member = 42; // Check for mutability
		}
		EQUAL_RANGE(r, (il<Elt>({ {42}, { 42 }, { 42 }, { 42 }, { 42 } })));
		for (auto&& elt : r)
		{
			static_assert(test::is_reference_v<decltype(elt)>, "elt is expected to be a reference");
			elt.member = 78; // Check for mutability
		}
		EQUAL_RANGE(r, (il<Elt>({ {78}, { 78 }, { 78 }, { 78 }, { 78 } })));
	}

	// Test return non-reference
	{
		std::vector<int> constVect{ 0, 1, 2, 3 };
		EQUAL_RANGE(
			constVect | transform([](auto a) {return a * 2; }),
			il<int>({ 0, 2, 4, 6 })
		);

		std::vector<Elt> vec = { {1} };
		auto r_copy = vec | transform([](auto a) {return Elt{ a.member + 1 }; });
		for (auto iter = std::begin(r_copy), end = std::end(r_copy); iter != end; ++iter)
		{
			CHECK(iter->member == 2); // Check for mutability
			CHECK((*iter).member == 2); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(*iter)> || 
				(test::is_reference_v<decltype(*iter)> == false),
				"*iter is not expected to be a reference");
		}
		for (auto&& elt : r_copy)
		{
			CHECK(elt.member == 2); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(elt)> || 
				(test::is_reference_v<decltype(elt)> == false),
				"elt is not expected to be a reference");
		}
		auto r_ref = vec | transform([](auto a) {return a.member; });
		for (auto iter = std::begin(r_ref), end = std::end(r_ref); iter != end; ++iter)
		{
			CHECK(*iter == 1); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(*iter)> || 
				(test::is_reference_v<decltype(*iter)> == false),
				"*iter is not expected to be a reference");
		}
		for (auto&& elt : r_ref)
		{
			CHECK(elt == 1); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(elt)> || 
				(test::is_reference_v<decltype(elt)> == false),
				"elt is not expected to be a reference");
		}
	}

	// **************************** divers compination test ***************************************

	EQUAL_RANGE(
		(iota(0, 3) | transform([](auto i) {return i * 2; }) | enumerate()),
		(il<std::pair<size_t, int>>{ {0, 0}, { 1, 2 }, { 2, 4 } })
	);

	std::vector<char> vec_abcd{ 'a', 'b', 'c', 'd' };
	EQUAL_RANGE(
		(vec_abcd | transform([](char i) {return i + 1; }) | enumerate()),
		(il<std::pair<size_t, char>>{ {0, 'b'}, { 1, 'c' }, { 2, 'd' }, { 3, 'e' } })
	);

	EQUAL_RANGE(
		(iota(0, 3000, 3) | transform([](auto i) {return i * 2; }) | enumerate() | slice(10, 13)),
		(il<std::pair<size_t, int>>{ {10, 60}, { 11, 66 }, { 12, 72 } })
	);

	EQUAL_RANGE(
		(zip(vec_abcd, iota(0, 4))),
		(il<std::tuple<char, int>>{ {'a', 0}, { 'b', 1 }, { 'c', 2 }, { 'd', 3 } })
	);

	EQUAL_RANGE(
		(iota(0, 100) | slice(0, 20) | stride(3)),
		(il<int>{0, 3, 6, 9, 12, 15, 18})
	);

	EQUAL_RANGE(
		(iota(10, 15) | retro()),
		(il<int>{14, 13, 12, 11, 10})
	);

	EQUAL_RANGE(
		(iota(0, 100) | slice(10, 15) | retro()),
		(il<int>{14, 13, 12, 11, 10})
	);

	EQUAL_RANGE(
		(iota(10, 15) | enumerate() | retro()),
		(il<std::tuple<size_t, int>>{ {4, 14}, { 3, 13 }, { 2, 12 }, { 1, 11 }, { 0, 10 }})
	);

	EQUAL_RANGE(
		(iota(0, 100) | enumerate() | slice(10, 15)),
		(il<std::tuple<size_t, int>>{ {10, 10}, { 11, 11 }, { 12, 12 }, { 13, 13 }, { 14, 14 } })
	);

	EQUAL_RANGE(
		(iota(0, 100) | enumerate() | slice(10, 15) | retro()),
		(il<std::tuple<size_t, int>>{ {14, 14}, { 13, 13 }, { 12, 12 }, { 11, 11 }, { 10, 10 } })
	);

	std::cout << "ALL TEST OK" << std::endl;

	return 0;
}
