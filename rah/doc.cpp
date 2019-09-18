//
// Copyright (c) 2019 Loïc HAMOT
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/// @struct rah::pipeable
/// @brief Allow to call a custom function when called whith the 'pipe' syntax on a range
/// @see rah::make_pipeable 

/// @fn rah::make_pipeable(MakeRange&& make_range)
/// @brief Call to create a "pipeable" function (UFCS style in c++)
///
/// @section create_pipeable How to create :
/// @snippet test.cpp make_pipeable create
/// @section use_pipeable How to use :
/// @snippet test.cpp make_pipeable use

/// @fn rah::make_iterator_range(I b, I e)
/// @brief Create a rah::iterator_range with two given iterators

/// @struct rah::iterator_facade
/// @brief Inerit to make an iterator
///
/// @tparam I is the type of iterator which inerit from iterator_facade
/// @tparam R is the type returned by dereferencing the iterator (It can be a reference type or not)
/// @tparam C is the iterator category (std::forward_iterator_tag, std::bidirectional_iterator_tag, std::random_access_iterator_tag)

/// @struct rah::iterator_facade<I, R, std::output_iterator_tag>
/// @brief Inerit to make an appendable range
///
/// Required to implement  :
/// @code{cpp}
/// template<typename V> void put(V&& value) const;
/// @endcode
/// @see rah::back_inserter
/// @see rah::stream_inserter

/// @struct rah::iterator_facade<I, R, std::forward_iterator_tag>
/// @brief Inerit to make a forward_iterator
///
/// Required to implement  :
/// @code{cpp}
/// void increment();
/// reference dereference() const;
/// bool equal(this_type other) const;
/// @endcode
/// @see rah::view::gererate_iterator

/// @struct rah::iterator_facade<I, R, std::bidirectional_iterator_tag>
/// @brief Inerit to make a bidirectional iterator
///
/// Required to implement  :
/// @code{cpp}
/// void increment();
/// void decrement();
/// reference dereference() const;
/// bool equal(this_type other) const;
/// @endcode
/// @see rah::view::zip_iterator

/// @struct rah::iterator_facade<I, R, std::random_access_iterator_tag>
/// @brief Inerit to make a random access iterator
///
/// Required to implement  :
/// @code{cpp}
/// void increment();
/// void advance(intptr_t val);
/// void decrement();
/// auto distance_to(this_type other);
/// reference dereference() const;
/// bool equal(this_type other) const;
/// @endcode
/// @see rah::view::iota_iterator

/// @fn rah::view::ints(T, T)
/// @brief Generate a range of monotonically increasing ints. When used without arguments, it generates the quasi-infinite range [0,1,2,3...]. It can also be called with a lower bound, or with a lower and upper bound (exclusive). [lower, uppder[
///
/// @snippet test.cpp ints

/// @fn rah::view::closed_ints(T, T)
/// @brief Generate a range of monotonically increasing ints. When used without arguments, it generates the quasi-infinite range [0,1,2,3...]. It can also be called with a lower bound, or with a lower and upper bound (inclusive).  [lower, uppder]
///
/// @snippet test.cpp closed_ints

/// @fn rah::view::iota(T, T, T)
/// @brief Generate a range of sequential integers, increasing by a defined step
///
/// @snippet test.cpp iota

/// @fn rah::view::repeat(V&&)
/// @brief Generate an infinite range of the given value
///
/// @snippet test.cpp repeat

/// @fn rah::view::generate(F&& func)
/// @brief Create an infinite range, repetitively calling func
///
/// @snippet test.cpp generate

/// @fn rah::view::generate_n(F&& func, size_t count)
/// @brief Create a range of N elemnts, repetitively calling func
///
/// @snippet test.cpp generate_n

/// @fn rah::view::all(R&& range)
/// @brief Create a view on the whole range

/// @fn rah::view::all()
/// @brief Create a view on the whole range
/// @remark pipeable syntax

/// @fn rah::view::transform(R&& range, F&& func)
/// @brief Create a view applying a transformation to each element of the input range
///
/// @snippet test.cpp rah::view::transform

/// @fn rah::view::transform(F&& func)
/// @brief Create a view applying a transformation to each element of the input range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::view::transform_pipeable

/// @fn rah::view::take(R&& range, size_t count)
/// @brief Given a source @b range and an integral @b count, return a range consisting of the first count elements from the source range, or the complete range if it has fewer elements.
///
/// @snippet test.cpp take

/// @fn rah::view::take(size_t count)
/// @brief Given a source @b range and an integral @b count, return a range consisting of the first count elements from the source range, or the complete range if it has fewer elements.
/// @remark pipeable syntax
///
/// @snippet test.cpp take_pipeable

/// @fn rah::view::drop(R&& range, size_t count)
/// @brief Given a source range and an integral count, return a range consisting of all but the first count elements from the source range, or an empty range if it has fewer elements. 
///
/// @snippet test.cpp drop

/// @fn rah::view::drop(size_t count)
/// @brief Given a source range and an integral count, return a range consisting of all but the first count elements from the source range, or an empty range if it has fewer elements. 
/// @remark pipeable syntax
///
/// @snippet test.cpp drop_pipeable

/// @fn rah::view::drop_exactly(R&& range, size_t count)
/// @brief Given a source range and an integral count, return a range consisting of all but the first count elements from the source range. The source range must have at least that many elements. 
///
/// @snippet test.cpp drop_exactly

/// @fn rah::view::drop_exactly(size_t count)
/// @brief Given a source range and an integral count, return a range consisting of all but the first count elements from the source range. The source range must have at least that many elements. 
/// @remark pipeable syntax
///
/// @snippet test.cpp drop_exactly_pipeable

/// @fn rah::view::counted(I&& it, size_t n, decltype(++it, 0) = 0)
/// @brief Given an iterator @b it and a count @b n, create a range that starts at @b it and includes the next @b n elements. 
///
/// @snippet test.cpp counted

/// @fn rah::view::counted(size_t count)
/// @brief Given an iterator @b it and a count @b n, create a range that starts at @b it and includes the next @b n elements. 
/// @remark pipeable syntax
///
/// @snippet test.cpp counted_pipeable

/// @fn rah::view::unbounded(I&& it)
/// @brief Given an iterator, return an infinite range that begins at that position. 
///
/// @snippet test.cpp unbounded

/// @fn rah::view::slice(R&& range, intptr_t begin, intptr_t end)
/// @brief Create a view that is a sub-range of a range
///
/// @snippet test.cpp slice

/// @fn rah::view::slice(intptr_t begin, intptr_t end)
/// @brief Create a view that is a sub-range of a range
/// @remark pipeable syntax
///
/// @snippet test.cpp slice_pipeable

/// @fn rah::view::stride(R&& range, size_t step)
/// @brief Create a view consisting of every Nth element, starting with the first
///
/// @snippet test.cpp stride

/// @fn rah::view::stride(size_t step)
/// @brief Create a view consisting of every Nth element, starting with the first
/// @remark pipeable syntax
///
/// @snippet test.cpp stride_pipeable

/// @fn rah::view::retro(R&& range)
/// @brief Create a view that traverses the source range in reverse order
///
/// @snippet test.cpp retro

/// @fn rah::view::retro()
/// @brief Create a view that traverses the source range in reverse order
/// @remark pipeable syntax
///
/// @snippet test.cpp retro_pipeable

/// @fn rah::view::zip(R&&... _ranges)
/// @brief Given N ranges, return a new range where Mth element is the result of calling std::make_tuple on the Mth elements of all N ranges. 
/// 
/// The resulting range has the size of the smaller of the sub-ranges
///
/// @snippet test.cpp zip

/// @fn rah::view::chunk(R&& range, size_t step)
/// @brief Create a view where each element is a range of N elements of the input range
///
/// @snippet test.cpp chunk

/// @fn rah::view::chunk(size_t step)
/// @brief Create a view where each element is a range of N elements of the input range
/// @remark pipeable syntax
///
/// @snippet test.cpp chunk_pipeable

/// @fn rah::view::filter(R&& range, F&& func)
/// @brief Create a view with only elements which are filtered
///
/// @snippet test.cpp filter

/// @fn rah::view::filter(F&& func)
/// @brief Create a view with only elements which are filtered
/// @remark pipeable syntax
///
/// @snippet test.cpp filter_pipeable

/// @fn rah::view::concat(R1&& range1, R2&& range2)
/// @brief Create a view that is the concatenation of 2 ranges
///
/// @snippet test.cpp concat

/// @fn rah::view::concat(R&& rightRange)
/// @brief Create a view that is the concatenation of 2 ranges
/// @remark pipeable syntax
///
/// @snippet test.cpp concat_pipeable

/// @fn rah::view::enumerate(R&& range)
/// @brief Pair each element of a range with its index. 
///
/// @snippet test.cpp enumerate

/// @fn rah::view::enumerate()
/// @brief Pair each element of a range with its index. 
/// @remark pipeable syntax
///
/// @snippet test.cpp enumerate_pipeable

/// @fn rah::view::map_value(R&& range)
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the first element of the pair. 
///
/// @snippet test.cpp map_value

/// @fn rah::view::map_value()
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the first element of the pair. 
/// @remark pipeable syntax
///
/// @snippet test.cpp map_value_pipeable

/// @fn rah::view::map_key(R&& range)
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the second element of the pair. 
///
/// @snippet test.cpp map_key

/// @fn rah::view::map_key()
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the second element of the pair. 
/// @remark pipeable syntax
///
/// @snippet test.cpp map_key_pipeable

/// @fn rah::view::single(V&& value)
/// @brief Given value, create a view containing one element
///
/// @snippet test.cpp single

/// @fn rah::view::join(R&& range_of_ranges)
/// @brief Given a range of ranges, join them into a flattened sequence of elements.
///
/// @snippet test.cpp join

/// @fn rah::view::join()
/// @brief Given a range of ranges, join them into a flattened sequence of elements.
/// @remark pipeable syntax
///
/// @snippet test.cpp join_pipeable

/// @fn rah::view::cycle(R&& range)
/// @brief Returns an infinite range that endlessly repeats the source range. 
///
/// @snippet test.cpp cycle

/// @fn rah::view::cycle()
/// @brief Returns an infinite range that endlessly repeats the source range. 
/// @remark pipeable syntax
///
/// @snippet test.cpp cycle_pipeable

/// @fn rah::view::for_each(R&& range, F&& func)
/// @brief Lazily applies an unary function to each element in the source range 
///        that returns another range (possibly empty), flattening the result. 
///
/// @snippet test.cpp for_each

/// @fn rah::view::for_each(F&& func)
/// @brief Lazily applies an unary function to each element in the source range 
///        that returns another range (possibly empty), flattening the result. 
/// @remark pipeable syntax
///
/// @snippet test.cpp for_each_pipeable


/*! \mainpage rah - A range (header only) library for C++
 *
 * # What is a range
 * A range is anything that can be iterate. Typically in C++ something is a range if we can call `begin(range)` and `end(range)` on it.
 *
 * # How to create a range
 * In **rah** this is done by `rah::iterator_range`.
 * Two way to create an iterator_range:
 * - `rah::iterator_range<iterator_type>(begin_iter, end_iter);`
 * - `rah::make_iterator_range (begin_iter, end_iter);`
 *
 * # What is a view
 * A view is a range returned by a function an which doesn't modify it's input range.\n
 * The computing is often done in a "lazy" way, that is to say at the moment of iteration.\n
 * There is two kind of view:
 * - Views which take a range and give a modified view on it:
 *   - Example: `rah::view::filter`, `rah::view::transform`
 * - Generators
 *   - Example: `rah::view::iota`, `rah::view::generate`
 *
 * # How to make a view
 * - To make a view you have to create an iterator.
 *   - For example the rah::view::filter view is an rah::iterator_range of rah::view::filter_iterator.
 * - Then you have to create a function taking a range and parameters (or no range for generator)
 *
* @code{.cpp}
template<typename R> auto retro(R&& range)
{
	return make_iterator_range(
		std::make_reverse_iterator(end(range)), std::make_reverse_iterator(begin(range)));
}
 * @endcode
 * - Then you can add a pipeable version of the function
 *
 * @code{.cpp}
auto retro()
{
	return make_pipeable([=](auto&& range) {return retro(range); });
}
 * @endcode
 *
 * # How to make an iterator
 * There is in **rah** an helper to create iterators: `rah::iterator_facade`
 * There are three kind of `rah::iterator_facade`:
 * - rah::iterator_facade<I, R, std::forward_iterator_tag>
 *   - For an example : rah::view::generate_iterator
 * - rah::iterator_facade<I, R, std::bidirectional_iterator_tag >
 *   - For an example : rah::view::zip_iterator
 * - rah::iterator_facade<I, R, std::random_access_iterator_tag>
 *   - For an example : rah::view::iota_iterator
 *
 * # How to make a pipeable view or algorithm
 *
 * Use the rah::make_pipeable function.
 * ## How to create a pipeable :
 * @snippet test.cpp make_pipeable create
 * ## How to use a pipeable :
 * @snippet test.cpp make_pipeable use
 *
 */

