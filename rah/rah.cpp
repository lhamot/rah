//
// Copyright (c) 2019 Loïc HAMOT
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
#include <list>
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

#undef assert
#define assert(CONDITION) \
std::cout << "assert : " << #CONDITION << std::endl; \
if(CONDITION) \
	std::cout << "OK" << std::endl; \
else \
	{std::cout << "NOT OK" << std::endl; abort();}

#define EQUAL_RANGE(RANGE, IL) \
std::cout << "assert : " << #RANGE << " = " << #IL << std::endl; \
if(rah::view::zip(RANGE, IL) | rah::all_of(PairEqual)) \
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

/// [make_pipeable create]
auto test_count(int i)
{
	return rah::make_pipeable([=](auto&& range) { return std::count(begin(range), end(range), i); });
}
/// [make_pipeable create]

int main()
{
	{
		/// [make_pipeable use]
		std::vector<int> vec{ 0, 1, 2, 2, 3 };
		assert((vec | test_count(2)) == 2);
		/// [make_pipeable use]
	}

	// *********************************** views **************************************************

	{
		/// [iota]
		std::vector<int> result;
		for (int i : rah::view::iota(10, 20, 2))
			result.push_back(i);
		assert(result == std::vector<int>({ 10, 12, 14, 16, 18 }));
		/// [iota]
	}

	{
		/// [generate]
		int y = 1;
		auto gen = rah::view::generate([&y]() mutable { auto prev = y; y *= 2; return prev; });
		std::vector<int> gen_copy;
		std::copy_n(begin(gen), 4, std::back_inserter(gen_copy));
		assert(gen_copy == std::vector<int>({ 1, 2, 4, 8 }));
		/// [generate]
	}
	{
		/// [generate_n]
		std::vector<int> result;
		int y = 1;
		for (int i : rah::view::generate_n([&y]() mutable { auto prev = y; y *= 2; return prev; }, 4))
			result.push_back(i);
		assert(result == std::vector<int>({ 1, 2, 4, 8 }));
		/// [generate_n]
	}

	// Test all
	EQUAL_RANGE((il<int>{0, 1, 2, 3} | rah::view::all()), (il<int>{ 0, 1, 2, 3 }));

	// Test transform
	{
		/// [rah::view::transform]
		std::vector<int> vec{ 0, 1, 2, 3 };
		std::vector<int> result;
		for (int i : rah::view::transform(vec, [](auto a) {return a * 2; }))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 2, 4, 6 }));
		/// [rah::view::transform]
	}
	{
		/// [rah::view::transform_pipeable]
		std::vector<int> vec{ 0, 1, 2, 3 };
		std::vector<int> result;
		for (int i : vec | rah::view::transform([](auto a) {return a * 2; }))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 2, 4, 6 }));
		/// [rah::view::transform_pipeable]
	}

	{
		/// [slice]
		std::vector<int> vec{ 0, 1, 2, 3, 4, 5, 6, 7 };
		std::vector<int> result;
		for (int i : rah::view::slice(vec, 2, 6))
			result.push_back(i);
		assert(result == std::vector<int>({ 2, 3, 4, 5 }));
		/// [slice]
	}
	{
		/// [slice_pipeable]
		std::vector<int> vec{ 0, 1, 2, 3, 4, 5, 6, 7 };
		std::vector<int> result;
		for (int i : vec | rah::view::slice(2, 6))
			result.push_back(i);
		assert(result == std::vector<int>({ 2, 3, 4, 5 }));
		/// [slice_pipeable]
	}

	{
		/// [stride]
		std::vector<int> vec{ 0, 1, 2, 3, 4, 5, 6, 7 };
		std::vector<int> result;
		for (int i : rah::view::stride(vec, 2))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 2, 4, 6 }));
		/// [stride]
	}
	{
		/// [stride_pipeable]
		std::vector<int> vec{ 0, 1, 2, 3, 4, 5, 6, 7 };
		std::vector<int> result;
		for (int i : vec | rah::view::stride(2))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 2, 4, 6 }));
		/// [stride_pipeable]
	}

	{
		/// [retro]
		std::vector<int> vec{ 0, 1, 2, 3 };
		std::vector<int> result;
		for (int i : rah::view::retro(vec))
			result.push_back(i);
		assert(result == std::vector<int>({ 3, 2, 1, 0 }));
		/// [retro]
	}
	{
		/// [retro_pipeable]
		std::vector<int> vec{ 0, 1, 2, 3 };
		std::vector<int> result;
		for (int i : vec | rah::view::retro())
			result.push_back(i);
		assert(result == std::vector<int>({ 3, 2, 1, 0 }));
		/// [retro_pipeable]
	}

	{
		/// [zip]
		std::vector<int> inputA{ 1, 2, 3, 4 };
		std::vector<double> inputB{ 2.5, 4.5, 6.5, 8.5 };
		std::vector<char> inputC{ 'a', 'b', 'c', 'd' };
		std::vector<std::tuple<int, double, char>> result;
		for (auto a_b_c : rah::view::zip(inputA, inputB, inputC))
			result.push_back(a_b_c);
		assert(result == (std::vector<std::tuple<int, double, char>>{
			{ 1, 2.5, 'a' },
			{ 2, 4.5, 'b' },
			{ 3, 6.5, 'c' },
			{ 4, 8.5, 'd' }
		}));
		/// [zip]
	}

	{
		/// [chunk]
		std::vector<int> vec_01234{ 0, 1, 2, 3, 4 };
		std::vector<std::vector<int>> result;
		for (auto elts : rah::view::chunk(vec_01234, 2))
			result.push_back(std::vector<int>(begin(elts), end(elts)));
		assert(result == std::vector<std::vector<int>>({ {0, 1}, { 2, 3 }, { 4 } }));
		/// [chunk]
	}
	{
		/// [chunk_pipeable]
		std::vector<int> vec_01234{ 0, 1, 2, 3, 4 };
		std::vector<std::vector<int>> result;
		for (auto elts : vec_01234 | rah::view::chunk(2))
			result.push_back(std::vector<int>(begin(elts), end(elts)));
		assert(result == std::vector<std::vector<int>>({ {0, 1}, { 2, 3 }, { 4 } }));
		/// [chunk_pipeable]
	}

	{
		/// [filter]
		std::vector<int> vec_01234{ 0, 1, 2, 3, 4 };
		std::vector<int> result;
		for (int i : rah::view::filter(vec_01234, [](auto a) {return a % 2 == 0; }))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 2, 4 }));
		/// [filter]
	}
	{
		/// [filter_pipeable]
		std::vector<int> vec_01234{ 0, 1, 2, 3, 4 };
		std::vector<int> result;
		for (int i : vec_01234 | rah::view::filter([](auto a) {return a % 2 == 0; }))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 2, 4 }));
		/// [filter_pipeable]
	}

	{
		/// [join]
		std::vector<int> inputA{ 0, 1, 2, 3 };
		std::vector<int> inputB{ 4, 5, 6 };
		std::vector<int> result;
		for (int i : rah::view::join(inputA, inputB))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 1, 2, 3, 4, 5, 6 }));
		/// [join]
	}
	{
		/// [join_pipeable]
		std::vector<int> inputA{ 0, 1, 2, 3 };
		std::vector<int> inputB{ 4, 5, 6 };
		std::vector<int> result;
		for (int i : inputA | rah::view::join(inputB))
			result.push_back(i);
		assert(result == std::vector<int>({ 0, 1, 2, 3, 4, 5, 6 }));
		/// [join_pipeable]
	}

	{
		/// [enumerate]
		std::vector<int> input{ 4, 5, 6, 7 };
		std::vector<std::tuple<size_t, int>> result;
		for (auto i_value : rah::view::enumerate(input))
			result.push_back(i_value);
		assert(result == (std::vector<std::tuple<size_t, int>>{ { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } }));
		/// [enumerate]
	}
	{
		/// [enumerate_pipeable]
		std::vector<int> input{ 4, 5, 6, 7 };
		std::vector<std::tuple<size_t, int>> result;
		for (auto i_value : input | rah::view::enumerate())
			result.push_back(i_value);
		assert(result == (std::vector<std::tuple<size_t, int>>{ { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } }));
		/// [enumerate_pipeable]
	}

	{
		/// [map_value]
		std::map<int, double> input{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 } };
		std::vector<double> result;
		for (double value : rah::view::map_value(input))
			result.push_back(value);
		assert(result == (std::vector<double>{ 1.5, 2.5, 3.5, 4.5 }));
		/// [map_value]
	}
	{
		/// [map_value_pipeable]
		std::map<int, double> input{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 } };
		std::vector<double> result;
		for (double value : input | rah::view::map_value())
			result.push_back(value);
		assert(result == (std::vector<double>{ 1.5, 2.5, 3.5, 4.5 }));
		/// [map_value_pipeable]
	}

	{
		/// [map_key]
		std::map<int, double> input{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 } };
		std::vector<int> result;
		for (int key : rah::view::map_key(input))
			result.push_back(key);
		assert(result == (std::vector<int>{ 1, 2, 3, 4 }));
		/// [map_key]
	}
	{
		/// [map_key_pipeable]
		std::map<int, double> input{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 } };
		std::vector<int> result;
		for (int key : input | rah::view::map_key())
			result.push_back(key);
		assert(result == (std::vector<int>{ 1, 2, 3, 4 }));
		/// [map_key_pipeable]
	}

	// *********************************** algos **************************************************

	{
		/// [rah::transform3]
		std::vector<int> vecIn1{ 0, 1, 2, 3 };
		std::vector<int> vecOut{ 0, 0, 0, 0 };
		rah::transform(vecIn1, vecOut, [](int a) {return a + 1; });
		assert(vecOut == std::vector<int>({ 1, 2, 3, 4 }));
		/// [rah::transform3]
	}
	{
		/// [rah::transform4]
		std::vector<int> vecIn1{ 0, 1, 2, 3 };
		std::vector<int> vecIn2{ 4, 3, 2, 1 };
		std::vector<int> vecOut{ 0, 0, 0, 0 };
		rah::transform(vecIn1, vecIn2, vecOut, [](int a, int b) {return a + b; });
		assert(vecOut == std::vector<int>({ 4, 4, 4, 4 }));
		/// [rah::transform4]
	}

	assert((rah::view::iota(0, 0) | rah::reduce(0, [](auto a, auto b) {return a + b; })) == 0);
	{
		/// [rah::reduce]
		std::vector<int> vecIn1{ 1, 2, 3, 4 };
		assert(rah::reduce(vecIn1, 0, [](auto a, auto b) {return a + b; }) == 10);
		/// [rah::reduce]
	}
	{
		/// [rah::reduce_pipeable]
		std::vector<int> vecIn1{ 1, 2, 3, 4 };
		assert((vecIn1 | rah::reduce(0, [](auto a, auto b) {return a + b; })) == 10);
		/// [rah::reduce_pipeable]
	}

	/// [rah::any_of]
	assert(rah::any_of(
		std::initializer_list<int>{ 3, 0, 1, 3, 4, 6 }, 
		[](auto a) {return a == 3; })
	);
	/// [rah::any_of]
	/// [rah::any_of_pipeable]
	assert((
		std::initializer_list<int>{0, 1, 2, 3, 4, 6}
		| rah::any_of([](auto a) {return a == 3; })
	));
	/// [rah::any_of_pipeable]
	assert((std::initializer_list<int>{3, 0, 1, 3, 4, 6} | rah::any_of([](auto a) {return a == 3; })));
	assert((std::initializer_list<int>{2, 0, 1, 2, 4, 6} | rah::any_of([](auto a) {return a == 3; })) == false);

	/// [rah::all_of]
	assert(rah::all_of(
		std::initializer_list<int>{ 4, 4, 4, 4 }, 
		[](auto a) {return a == 4; })
	);
	/// [rah::all_of]
	assert(rah::all_of(std::initializer_list<int>{ 4, 4, 3, 4 }, [](auto a) {return a == 4; }) == false);
	assert((std::initializer_list<int>{ 4, 4, 4, 4 } | rah::all_of([](auto a) {return a == 4; })));
	/// [rah::all_of_pipeable]
	assert((
		std::initializer_list<int>{ 4, 4, 3, 4 } 
	    | rah::all_of([](auto a) {return a == 4; })
	) == false);
	/// [rah::all_of_pipeable]

	/// [rah::none_of]
	assert((rah::none_of(
		std::initializer_list<int>{7, 8, 9, 10}, 
		[](auto a) {return a == 11; })
	));
	/// [rah::none_of]
	assert((std::initializer_list<int>{7, 8, 9, 10} | rah::none_of([](auto a) {return a == 11; })));
	/// [rah::none_of_pipeable]
	assert((
		std::initializer_list<int>{7, 8, 9, 10, 11} 
	    | rah::none_of([](auto a) {return a == 11; })
	) == false);
	/// [rah::none_of_pipeable]

	/// [rah::count]
	assert(rah::count(std::initializer_list<int>{ 4, 4, 4, 3 }, 3) == 1);
	/// [rah::count]
	/// [rah::count_pipeable]
	assert((std::initializer_list<int>{ 4, 4, 4, 3 } | rah::count(4)) == 3);
	/// [rah::count_pipeable]

	/// [rah::count_if]
	assert(rah::count_if(il<int>{ 4, 4, 4, 3 }, [](auto a) {return a == 4; }) == 3);
	/// [rah::count_if]
	/// [rah::count_if_pipeable]
	assert((std::initializer_list<int>{ 4, 4, 4, 3 } | rah::count_if([](auto a) {return a == 3; })) == 1);
	/// [rah::count_if_pipeable]

	{
		/// [rah::for_each]
		std::vector<int> testFE{ 4, 4, 4, 4 };
		rah::for_each(testFE, [](auto& value) {return ++value; });
		EQUAL_RANGE(testFE, il<int>({ 5, 5, 5, 5 }));
		/// [rah::for_each]
	}
	{
		/// [rah::for_each_pipeable]
		std::vector<int> testFE{ 4, 4, 4, 4 };
		testFE | rah::for_each([](auto& value) {return ++value; });
		EQUAL_RANGE(testFE, il<int>({ 5, 5, 5, 5 }));
		/// [rah::for_each_pipeable]
	}

	{
		/// [rah::to_container_pipeable]
		std::vector<std::pair<int, char>> in1{ {4, 'a'}, { 5, 'b' }, { 6, 'c' }, { 7, 'd' } };
		std::map<int, char> map_4a_5b_6c_7d = in1 | rah::to_container<std::map<int, char>>();
		assert(
			map_4a_5b_6c_7d == (std::map<int, char>{ {4, 'a'}, { 5, 'b' }, { 6, 'c' }, { 7, 'd' } })
		);

		std::list<int> in2{ 4, 5, 6, 7 };
		std::vector<int> out = in2 | rah::to_container<std::vector<int>>();
		assert(out == (std::vector<int>{ 4, 5, 6, 7 }));
		/// [rah::to_container_pipeable]
	}
	{
		/// [rah::to_container]
		std::vector<std::pair<int, char>> in1{ {4, 'a'}, { 5, 'b' }, { 6, 'c' }, { 7, 'd' } };
		std::map<int, char> map_4a_5b_6c_7d = rah::to_container<std::map<int, char>>(in1);
		assert(
			map_4a_5b_6c_7d == (std::map<int, char>{ {4, 'a'}, { 5, 'b' }, { 6, 'c' }, { 7, 'd' } })
		);

		std::list<int> in2{ 4, 5, 6, 7 };
		std::vector<int> out = rah::to_container<std::vector<int>>(in2);
		assert(out == (std::vector<int>{ 4, 5, 6, 7 }));
		/// [rah::to_container]
	}

	{
		/// [rah::mismatch]
		std::vector<int> in1 = { 1, 2, 3, 4 };
		std::vector<int> in2 = { 1, 2, 42, 42 };
		auto r1_r2 = rah::mismatch(in1, in2);
		std::vector<int> out1;
		std::vector<int> out2;
		std::copy(begin(std::get<0>(r1_r2)), end(std::get<0>(r1_r2)), std::back_inserter(out1));
		std::copy(begin(std::get<1>(r1_r2)), end(std::get<1>(r1_r2)), std::back_inserter(out2));
		assert(out1 == std::vector<int>({ 3, 4 }));
		assert(out2 == std::vector<int>({ 42, 42 }));
		/// [rah::mismatch]
	}

	{
		/// [rah::find]
		assert(
			(rah::find(std::vector<int>{ 1, 2, 3, 4 }, 3) | rah::equal(std::initializer_list<int>({3, 4})))
		);
		/// [rah::find]
		/// [rah::find_pipeable]
		assert(
			(std::vector<int>{ 1, 2, 3, 4 } | rah::find(3) | rah::equal(std::initializer_list<int>({ 3, 4 })))
		);
		/// [rah::find_pipeable]
		/// [rah::find_if]
		assert(
			(rah::find_if(std::vector<int>{ 1, 2, 3, 4 }, [](int i) {return i == 3; }) 
				| rah::equal(std::initializer_list<int>({ 3, 4 })))
		);
		/// [rah::find_if]
		/// [rah::find_if_pipeable]
		assert(
			(std::vector<int>{ 1, 2, 3, 4 } | rah::find_if([](int i) {return i == 3; }) 
				| rah::equal(std::initializer_list<int>({ 3, 4 })))
		);
		/// [rah::find_if_pipeable]
		/// [rah::find_if_not]
		assert(
			(rah::find_if_not(std::vector<int>{ 1, 2, 3, 4 }, [](int i) {return i < 3; }) 
				| rah::equal(std::initializer_list<int>({ 3, 4 })))
		);
		/// [rah::find_if_not]
		/// [rah::find_if_not_pipeable]
		assert(
			(std::vector<int>{ 1, 2, 3, 4 } | rah::find_if_not([](int i) {return i < 3; }) 
				| rah::equal(std::initializer_list<int>({ 3, 4 })))
		);
		/// [rah::find_if_not_pipeable]
	}

	{
		/// [rah::size]
		std::vector<int> vec3{ 1, 2, 3 };
		assert(rah::size(vec3) == 3);
		/// [rah::size]
	}
	{
		/// [rah::size_pipeable]
		std::vector<int> vec3{ 1, 2, 3 };
		assert((vec3 | rah::size()) == 3);
		/// [rah::size_pipeable]
	}

	{
		/// [rah::equal]
		std::vector<int> in1{ 1, 2, 3 };
		std::vector<int> in2{ 1, 2, 3 };
		std::vector<int> in3{ 11, 12, 13 };
		assert(rah::equal(in1, in2));
		assert(rah::equal(in1, in3) == false);
		/// [rah::equal]
	}
	{
		/// [rah::equal_pipeable]
		std::vector<int> in1{ 1, 2, 3 };
		std::vector<int> in2{ 1, 2, 3 };
		std::vector<int> in3{ 11, 12, 13 };
		assert(in1 | rah::equal(in2));
		assert(not (in1 | rah::equal(in3)));
		/// [rah::equal_pipeable]
	}

	/// [rah::empty]
	assert(not (rah::empty(std::vector<int>{ 1, 2, 3 })));
	assert(rah::empty(std::vector<int>()));
	/// [rah::empty]
	/// [rah::empty_pipeable]
	assert(not (std::vector<int>{ 1, 2, 3 } | rah::empty()));
	assert(std::vector<int>() | rah::empty());
	/// [rah::empty_pipeable]

	{
		/// [rah::copy]
		std::vector<int> in{ 1, 2, 3 };
		std::vector<int> out{ 0, 0, 0, 4, 5 };
		assert(rah::copy(in, out) | rah::equal(std::initializer_list<int>({ 4, 5 })));
		assert(out == (std::vector<int>{ 1, 2, 3, 4, 5 }));
		/// [rah::copy]
	}
	{
		/// [rah::copy_into]
		std::vector<int> in{ 1, 2, 3 };
		std::vector<int> out{ 0, 0, 0, 4, 5 };
		assert((in | rah::copy_into(out) | rah::equal(std::initializer_list<int>({ 4, 5 }))));
		assert(out == (std::vector<int>{ 1, 2, 3, 4, 5 }));
		/// [rah::copy_into]
	}

	// ********************************* test return ref and non-ref ******************************

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

	// Test return reference

	{
		std::vector<Elt> vec = { {0}, { 1 }, { 2 }, { 3 }, { 4 } };
		auto& r = vec;
		for (auto iter = begin(r), end_iter = end(r); iter != end_iter; ++iter)
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
		for (auto iter = begin(r_copy), end_iter = end(r_copy); iter != end_iter; ++iter)
		{
			assert(iter->member == 2); // Check for mutability
			assert((*iter).member == 2); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(*iter)> || 
				(test::is_reference_v<decltype(*iter)> == false),
				"*iter is not expected to be a reference");
		}
		for (auto&& elt : r_copy)
		{
			assert(elt.member == 2); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(elt)> || 
				(test::is_reference_v<decltype(elt)> == false),
				"elt is not expected to be a reference");
		}
		auto r_ref = vec | transform([](auto a) {return a.member; });
		for (auto iter = begin(r_ref), end_iter = end(r_ref); iter != end_iter; ++iter)
		{
			assert(*iter == 1); // Check for mutability
			static_assert(test::is_rvalue_reference_v<decltype(*iter)> || 
				(test::is_reference_v<decltype(*iter)> == false),
				"*iter is not expected to be a reference");
		}
		for (auto&& elt : r_ref)
		{
			assert(elt == 1); // Check for mutability
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
