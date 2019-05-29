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

template<typename T>
struct WhatIsIt;

int main()
{
	using namespace rah;
	using namespace rah::lazy;
	using namespace std;

	// ********************** Non-modifying sequence operations ***********************************

	// Test all_of
	CHECK(all_of(il<int>{ 4, 4, 4, 4 }, [](auto a) {return a == 4; }));
	CHECK(all_of(il<int>{ 4, 4, 3, 4 }, [](auto a) {return a == 4; }) == false);
	CHECK((il<int>{ 4, 4, 4, 4 } | all_of([](auto a) {return a == 4; })));
	CHECK((il<int>{ 4, 4, 3, 4 } | all_of([](auto a) {return a == 4; })) == false);

	// Test any_of
	CHECK((il<int>{3, 0, 1, 3, 4, 6} | any_of([](auto a) {return a == 3; })));
	CHECK((il<int>{2, 0, 1, 3, 4, 6} | any_of([](auto a) {return a == 3; })));
	CHECK((il<int>{2, 0, 1, 2, 4, 6} | any_of([](auto a) {return a == 3; })) == false);

	// Test none_of
	CHECK((il<int>{7, 8, 9, 10} | none_of([](auto a) {return a == 11; })));
	CHECK((il<int>{7, 8, 9, 10, 11} | none_of([](auto a) {return a == 11; })) == false);

	// foreach
	{
		std::vector<int> testFE{ 4, 4, 4, 4 };
		for_each(testFE, [](auto& value) {return ++value; });
		EQUAL_RANGE(testFE, il<int>({ 5, 5, 5, 5 }));
		testFE | for_each([](auto& value) {return ++value; });
		EQUAL_RANGE(testFE, il<int>({ 6, 6, 6, 6 }));
	}

	// count
	CHECK((il<int>{ 4, 4, 4, 3 } | count(4)) == 3);
	CHECK(count(il<int>{ 4, 4, 4, 3 }, 3) == 1);

	// count_if
	CHECK((il<int>{ 4, 4, 4, 3 } | count_if([](auto a) {return a == 3; })) == 1);
	CHECK(count_if(il<int>{ 4, 4, 4, 3 }, [](auto a) {return a == 4; }) == 3);

	// mismatch
	{
		auto a = { 1, 2, 3, 4 };
		auto b = { 1, 2, 42, 42 };
		auto[range1, range2] = mismatch(a, b);
		EQUAL_RANGE(range1, il<int>({ 3, 4 }));
		EQUAL_RANGE(range2, il<int>({ 42, 42 }));
	}

	// find
	{
		EQUAL_RANGE(find(il<int>{ 1, 2, 3, 4 }, 3), il<int>({3, 4}));
		EQUAL_RANGE(find_if(il<int>{ 1, 2, 3, 4 }, [](int i) {return i == 3; }), il<int>({ 3, 4 }));
		EQUAL_RANGE(find_if_not(il<int>{ 1, 2, 3, 4 }, [](int i) {return i < 3; }), il<int>({ 3, 4 }));
	}

	// ***************************** Generators ***************************************************

	// Test iota
	EQUAL_RANGE(iota(0, 4), il<int>({ 0, 1, 2, 3 }));
	EQUAL_RANGE(iota(10, 20, 2), il<int>({ 10, 12, 14, 16, 18 }));

	// ***************************** Adaptors *****************************************************

	// Test transform
	EQUAL_RANGE(iota(0, 4) | transform([](auto a) {return a * 2; }), il<int>({ 0, 2, 4, 6 }));

	// Test transform
	{
		std::vector<int> constVect{ 0, 1, 2, 3 };
		EQUAL_RANGE((constVect | transform([](auto a) {return a * 2; })), il<int>({ 0, 2, 4, 6 }));
	}

	// Test slice
	EQUAL_RANGE(iota(0, 100) | slice(10, 15), il<int>({ 10, 11, 12, 13, 14 }));

	// Test stride
	EQUAL_RANGE(iota(0, 100) | stride(20), il<int>({ 0, 20, 40, 60, 80 }));

	// Test retro
	EQUAL_RANGE(iota(0, 4) | retro(), il<int>({ 3, 2, 1, 0 }));

	// Test zip
	using tIDC = std::tuple<int, double, char>;
	EQUAL_RANGE(zip(il<int>{ 1, 2, 3, 4 }, il<double>{ 2.5, 4.5, 6.5, 8.5 }, il<char>{ 'a', 'b', 'c', 'd' }),
		(il<tuple<int, double, char>>{ tIDC{ 1, 2.5, 'a' }, tIDC{ 2, 4.5, 'b' }, tIDC{ 3, 6.5, 'c' }, tIDC{ 4, 8.5, 'd' } }));

	// Test filter
	EQUAL_RANGE(iota(0, 8) | filter([](auto a) {return a % 2 == 0; }), il<int>({ 0, 2, 4, 6 }));

	// Test enumerate
	using tUII = std::tuple<unsigned int, int>;
	EQUAL_RANGE(iota(4, 8) | enumerate(), (il<tuple<unsigned int, int>>{ tUII{ 0, 4 }, tUII{ 1, 5 }, tUII{ 2, 6 }, tUII{ 3, 7 } }));

	// Test mapValue
	EQUAL_RANGE((std::map<int, double>{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 }} | mapValue()), (il<double>{ 1.5, 2.5, 3.5, 4.5 }));

	// Test mapKey
	EQUAL_RANGE((std::map<int, double>{ {1, 1.5}, { 2, 2.5 }, { 3, 3.5 }, { 4, 4.5 }} | mapKey()), (il<int>{ 1, 2, 3, 4 }));

	// ******************************** Reduce ****************************************************

	// Test reduce
	CHECK((iota(1, 5) | reduce(0, [](auto a, auto b) {return a + b; })) == 10);

	// Test one_of
	CHECK(((il<int>{0, 1, 1} | count(1)) == 2));

	CHECK(((il<int>{0, 1, 1} | count_if([](auto a) {return a == 1; })) == 2));

	// ******************* test to_container *******************************

	{
		auto&& vec_4_5_6_7 = iota(4, 8) | to_container<std::vector<int>>();
		EQUAL_RANGE(vec_4_5_6_7, (il<int>{ 4, 5, 6, 7 }));

		auto toPair = [](auto ab) {return std::make_pair(std::get<0>(ab), std::get<1>(ab)); };
		auto&& map_4a_5b_6c_7d = zip(iota(4, 8), iota('a', 'e')) | transform(toPair) | to_container<std::map<int, char>>();
		EQUAL_RANGE(map_4a_5b_6c_7d, (il<std::pair<int const, char>>{ {4, 'a'}, { 5, 'b' }, { 6, 'c' }, { 7, 'd' } }));
	}

	// *********************** generate ***********************************************************

	EQUAL_RANGE(generate([y = 1]() mutable { auto prev = y; y *= 2; return prev; }) | slice(0, 4), (il<int>{1, 2, 4, 8}));
	EQUAL_RANGE(generate_n([y = 1]() mutable { auto prev = y; y *= 2; return prev; }, 4), (il<int>{1, 2, 4, 8}));

	// **************************************** chunk *********************************************

	{
		auto to_test = il<int>{ 0, 1, 2, 3, 4 } | chunk(2);
		auto ref = il<il<int>>{ {0, 1}, { 2, 3 }, { 4 } };
		CHECK(size(to_test) == size(ref));
		for (auto elt : zip(to_test, ref))
		{
			CHECK(size(std::get<0>(elt)) == size(std::get<1>(elt)));
			CHECK(equal(std::get<0>(elt), std::get<1>(elt)));
		}
	}

	// ************************************** join ************************************************

	EQUAL_RANGE((il<int>{ 0, 1, 2, 3 } | join(il<int>{ 4, 5, 6 })), (il<int>{0, 1, 2, 3, 4, 5, 6}));

	// ******************* test output_interator *******************************
	std::forward_list<int> tutu = { 101, 102, 103, 104 };
	for (auto i : tutu | transform([](int i) {return i * 2; }))
		std::cout << i << std::endl;

	std::map<int, char> testMap = {
		{ 1, 'a' },
	{ 2, 'e' },
	{ 3, 'i' },
	{ 4, 'o' },
	{ 5, 'u' },
	};

	std::cout << "iota(0, 10) | filter([](auto i){return i % 2 == 0;})" << std::endl;
	for (size_t i : iota(0, 10) | filter([](auto i) {return i % 2 == 0; }))
		std::cout << i << std::endl;

	std::cout << "mapValue()" << std::endl;
	for (auto i : testMap | mapValue())
		std::cout << i << std::endl;

	std::cout << "mapKey()" << std::endl;
	for (auto i : testMap | mapKey())
		std::cout << i << std::endl;

	std::cout << "iota(0, 10)" << std::endl;
	for (size_t i : iota(0, 10))
		std::cout << i << std::endl;

	std::cout << "enumerate(iota(10, 20))" << std::endl;
	for (auto[index, value] : enumerate(iota(10, 20)))
		std::cout << index << " " << value << std::endl;

	std::cout << "transform(iota(0, 10), [](int i) {return i * 2; })" << std::endl;
	auto r1 = iota(0, 10);
	auto func = [](int i) {return i * 2; };
	auto r2 = transform(r1, func);
	for (size_t i : r2)
		std::cout << i << std::endl;

	std::cout << "transform(iota(0, 10), [](int i) {return i * 2; })" << std::endl;
	for (size_t i : transform(iota(0, 10), [](int i) {return i * 2; }))
		std::cout << i << std::endl;

	std::cout << "iota(0, 10) | transform([](int i) {return i * 2; })" << std::endl;
	for (size_t i : iota(0, 10) | transform([](int i) {return i * 2; }))
		std::cout << i << std::endl;

	std::cout << "iota(0, 10) | transform([](int i) {return i * 2; }) | enumerate()" << std::endl;
	for (auto[index, value] : iota(0, 10) | transform([](int i) {return i * 2; }) | enumerate())
		std::cout << index << " " << value << std::endl;

	std::vector<char> toto = { 'a', 'b', 'c', 'd' };
	std::cout << "toto | transform([](int i) {return i * 2; }) | enumerate()" << std::endl;
	for (auto[index, value] : toto | transform([](char i) {return i * 2; }) | enumerate())
		std::cout << index << " " << value << std::endl;

	std::cout << "iota(0, 100) | slice(10, 20)" << std::endl;
	for (size_t i : iota(0, 100) | slice(10, 20))
		std::cout << i << std::endl;

	std::cout << "iota(0, 3000, 3) | transform([](int i) {return i * 2; }) | enumerate() | slice(10, 20)" << std::endl;
	for (auto[index, value] : iota(0, 3000, 3) | transform([](int i) {return i * 2; }) | enumerate() | slice(10, 20))
		std::cout << index << " " << value << std::endl;

	std::cout << "zip(toto, iota(0, 4))" << std::endl;
	for (auto[a, b] : zip(toto, iota(0, 4)))
		std::cout << a << " " << b << std::endl;

	std::cout << "iota(0, 100) | slice(0, 20) | stride(3)" << std::endl;
	for (size_t i : iota(0, 100) | slice(0, 20) | stride(3))
		std::cout << i << std::endl;

	std::cout << "iota(10, 20) | retro()" << std::endl;
	for (size_t i : iota(10, 20) | retro())
		std::cout << i << std::endl;

	std::cout << "iota(0, 100) | slice(10, 20) | retro()" << std::endl;
	for (size_t i : iota(0, 100) | slice(10, 20) | retro())
		std::cout << i << std::endl;

	std::cout << "iota(10, 20) | enumerate() | retro()" << std::endl;
	for (auto[index, value] : iota(10, 20) | enumerate() | retro())
		std::cout << index << " " << value << std::endl;

	std::cout << "iota() | enumerate() | slice(10, 20)" << std::endl;
	for (auto[index, value] : iota(0, 100) | enumerate() | slice(10, 20))
		std::cout << index << " " << value << std::endl;

	std::cout << "iota() | enumerate() | slice(10, 20) | retro()" << std::endl;
	for (auto[index, value] : iota(0, 100) | enumerate() | slice(10, 20) | retro())
		std::cout << index << " " << value << std::endl;

	std::cout << "iota() | enumerate() | slice(10, 20) | retro()" << std::endl;
	for (auto[index, value] : iota(0, 100) | enumerate() | slice(10, 20) | retro())
		std::cout << index << " " << value << std::endl;

	return 0;
}
