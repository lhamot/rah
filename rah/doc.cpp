#include <iostream>
#include <algorithm>
#include <vector>
#include "rah.hpp"

/// @fn rah::make_pipeable(MakeRange&& make_range)
/// @brief Call to create a "pipeable" function (UFCS style in c++)
///
/// @snippet doc.cpp make_pipeable create
/// @snippet doc.cpp make_pipeable use

/// @class rah::iterator_facade<I, R, std::forward_iterator_tag>
/// @brief Inerit to make a forward_iterator
///
/// Require to implement 
/// @code{cpp}
/// void increment();
/// reference dereference() const;
/// bool equal(this_type other) const;
/// @endcode
/// @see rah::view::gererate_iterator

/// @class rah::iterator_facade<I, R, std::bidirectional_iterator_tag>
/// @brief Inerit to make a bidirectional iterator
///
/// Require to implement 
/// @code{cpp}
/// void increment();
/// void decrement();
/// reference dereference() const;
/// bool equal(this_type other) const;
/// @endcode
/// @see rah::view::zip_iterator

/// @class rah::iterator_facade<I, R, std::random_access_iterator_tag>
/// @brief Inerit to make a random access iterator
///
/// Require to implement 
/// @code{cpp}
/// void increment();
/// void advance(intptr_t val);
/// void decrement();
/// auto distance_to(this_type other);
/// reference dereference() const;
/// bool equal(this_type other) const;
/// @endcode
/// @see rah::view::iota_iterator

/// @fn rah::view::iota(T, T, T)
/// @brief Generate a range of sequentially increasing integers
///
/// @snippet doc.cpp iota

/// @fn rah::view::generate(F&& func)
/// @brief Create an infinite range, repetitively calling func
///
/// @snippet doc.cpp generate

/// @fn rah::view::generate_n(F&& func, size_t count)
/// @brief Create a range of N elemnts, repetitively calling func
///
/// @snippet doc.cpp generate_n

/// @fn rah::view::all(R&& range)
/// @brief Create a view on the whole range

/// @fn rah::view::all()
/// @brief Create a view on the whole range
/// @remark pipeable syntax

/// @fn rah::view::transform(R&& range, F&& func)
/// @brief Create a range applying a transformation to each element of the input range
/// 
/// @snippet doc.cpp rah::view::transform

/// @fn rah::view::transform(F&& func)
/// @brief Create a range applying a transformation to each element of the input range
/// @remark pipeable syntax
/// 
/// @snippet doc.cpp rah::view::transform_pipeable

/// [make_pipeable create]
auto count(int i)
{
	return rah::make_pipeable([=](auto&& range) { return std::count(std::begin(range), std::end(range), i); });
}
/// [make_pipeable create]

void doc_code()
{
	{
		/// [make_pipeable use]
		std::vector<int> vec{ 0, 1, 2, 2, 3 };
		std::cout << (vec | count(2)) << std::endl;
		// 2
		/// [make_pipeable use]
	}

	/// [iota]
	for(int i: rah::view::iota(0, 10, 2))
	    std::cout << i << " ";
	// 0 2 4 6 8
	/// [iota]
	std::cout << std::endl;

	{
		/// [generate]
		auto gen = rah::view::generate([y = 1]() mutable { auto prev = y; y *= 2; return prev; });
		std::vector<int> gen_copy;
		std::copy_n(std::begin(gen), 4, std::back_inserter(gen_copy));
		for (int i : gen_copy)
			std::cout << i << " ";
		// 1 2 4 8
		/// [generate]
		std::cout << std::endl;
	}

	/// [generate_n]
	for (int i : rah::view::generate_n([y = 1]() mutable { auto prev = y; y *= 2; return prev; }, 4))
		std::cout << i << " ";
	// 1 2 4 8
	/// [generate_n]
	std::cout << std::endl;

	{
		/// [rah::view::transform]
		std::vector<int> vec{ 0, 1, 2, 3 };
		for (int i : rah::view::transform(vec, [](auto a) {return a * 2; }))
			std::cout << i << " ";
		// 0 2 4 6
		/// [rah::view::transform]
		std::cout << std::endl;
	}

	{
		/// [rah::view::transform_pipeable]
		std::vector<int> vec{ 0, 1, 2, 3 };
		for (int i : vec | rah::view::transform([](auto a) {return a * 2; }))
			std::cout << i << " ";
		// 0 2 4 6
		/// [rah::view::transform_pipeable]
		std::cout << std::endl;
	}
}
