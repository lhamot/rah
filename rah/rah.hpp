﻿//
// Copyright (c) 2019 Loïc HAMOT
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#ifdef MSVC
#pragma warning(push, 0)
#endif
#include <type_traits>
#include <iterator>
#include <cassert>
#include <tuple>
#include <algorithm>
#include <numeric>
#ifdef MSVC
#pragma warning(pop)
#endif

#ifndef RAH_NAMESPACE
#define RAH_NAMESPACE rah
#endif

namespace RAH_NAMESPACE
{

// **************************** range traits ******************************************************

template<typename T> T& fake() { return *((std::remove_reference_t<T>*)nullptr); }

template<typename T>
using range_begin_type_t = decltype(begin(fake<T>()));

template<typename T>
using range_end_type_t = decltype(end(fake<T>()));

template<typename T>
using range_ref_type_t = decltype(*begin(fake<T>()));

template<typename T>
using range_value_type_t = std::remove_reference_t<range_ref_type_t<T>>;

template<typename R>
using range_iter_categ_t = typename std::iterator_traits<range_begin_type_t<R>>::iterator_category;

// ******************************** iterator_range ************************************************

template<typename I>
struct iterator_range
{
	I begin_iter;
	I end_iter;

	I begin() const { return begin_iter; }
	I begin() { return begin_iter; }
	I end() const { return end_iter; }
	I end() { return end_iter; }
};

template<typename I>
auto make_iterator_range(I b, I e)
{
	return iterator_range<I>{b, e};
}

/// Get the begin iterator of the range
template<typename I> I begin(iterator_range<I>& r) { return r.begin_iter; }
/// Get the "past the" end iterator of the range
template<typename I> I end(iterator_range<I>& r) { return r.end_iter; }
/// Get the begin iterator of the range
template<typename I> I begin(iterator_range<I> const& r) { return r.begin_iter; }
/// Get the "past the" end iterator of the range
template<typename I> I end(iterator_range<I> const& r) { return r.end_iter; }

// **************************************** pipeable **********************************************

template<typename Func>
struct pipeable
{
	Func func;
};

template<typename MakeRange>
auto make_pipeable(MakeRange&& make_range)
{
	return pipeable<MakeRange>{ make_range };
}

template<typename R, typename MakeRange>
auto operator | (R&& range, pipeable<MakeRange> const& adapter)
{
	return adapter.func(std::forward<R>(range));
}

// ************************************ iterator_facade ********************************************

template<typename I, typename R, typename C> struct iterator_facade;

template<typename I, typename R> 
struct iterator_facade<I, R, std::forward_iterator_tag>
{
	template <class Reference>
	struct pointer_type
	{
		struct proxy
		{
			Reference m_ref;
			Reference const* operator->() { return &m_ref; }
			operator Reference const*() { return &m_ref; }
		};
		typedef proxy type;
	};

	template <class T>
	struct pointer_type<T&> // "real" references
	{
		using type = T*;
	};

	using iterator_category = std::forward_iterator_tag;
	using value_type = std::remove_reference_t<R>;
	using difference_type = intptr_t;
	typedef typename pointer_type<R>::type pointer;
	using reference = R;

	static_assert(std::is_reference<value_type>::value == false, "value_type can't be a reference");

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator++()
	{
		self().increment();
		return *this;
	}

	auto operator*() const { return self().dereference(); }
	auto operator->() const { return pointer{ self().dereference() }; }
	bool operator!=(I other) const { return self().equal(other) == false; }
	bool operator==(I other) const { return self().equal(other); }
};

template<typename I, typename R>
struct iterator_facade<I, R, std::bidirectional_iterator_tag> : iterator_facade<I, R, std::forward_iterator_tag>
{
	using iterator_category = std::bidirectional_iterator_tag;

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator--()
	{
		self().decrement();
		return *this;
	}
};

template<typename I, typename R>
struct iterator_facade<I, R, std::random_access_iterator_tag> : iterator_facade<I, R, std::bidirectional_iterator_tag>
{
	using iterator_category = std::random_access_iterator_tag;

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator+=(intptr_t increment)
	{
		self().advance(increment);
		return *this;
	}

	auto operator+(intptr_t increment)
	{
		auto iter = self();
		iter.advance(increment);
		return iter;
	}

	auto& operator-=(intptr_t increment)
	{
		self().advance(-increment);
		return *this;
	}

	auto operator-(intptr_t increment)
	{
		auto iter = self();
		iter.advance(-increment);
		return iter;
	}

	auto operator-(I other) const { return self().distance_to(other); }
	bool operator<(I other) const { return distance_to(other) < 0; }
	bool operator<=(I other) const { return distance_to(other) <= 0; }
	bool operator>(I other) const { return distance_to(other) > 0; }
	bool operator>=(I other) const { return distance_to(other) >= 0; }
	auto operator[](intptr_t increment) const { return *(self() + increment); }
};

namespace view
{

// ********************************** iota ********************************************************

template<typename T = size_t>
struct iota_iterator : iterator_facade<iota_iterator<T>, T, std::random_access_iterator_tag>
{
	T val_;
	T step_;
	
	iota_iterator(T val, T step) : val_(val), step_(step) {}

	void increment() { val_ += step_; }
	void advance(intptr_t value) { val_ += T(step_ * value); }
	void decrement() { val_ -= step_; }
	auto distance_to(iota_iterator other) const { return (val_ - other.val_) / step_; }
	auto dereference() const { return val_; }
	bool equal(iota_iterator other) const { return val_ == other.val_; }
};

template<typename T = size_t> auto iota(T b, T e, T step = 1)
{
	assert(step != 0);
	return iterator_range<iota_iterator<T>>{ { b, step}, { e, step }};
}

// ********************************** generate ****************************************************

template<typename F>
struct gererate_iterator : iterator_facade<gererate_iterator<F>, decltype(fake<F>()()), std::forward_iterator_tag>
{
	mutable F func_;
	size_t iter_count_ = 0;
	
	gererate_iterator(F const& func, size_t iter_count = 0) : func_(func), iter_count_(iter_count) {}

	void increment() { ++iter_count_; }
	auto dereference() const { return func_(); }
	bool equal(gererate_iterator other) const { return iter_count_ == other.iter_count_; }
};

template<typename F> auto generate(F&& func)
{
	return iterator_range<gererate_iterator<F>>{ { func}, { func }};
}

template<typename F> auto generate_n(F&& func, size_t count)
{
	return iterator_range<gererate_iterator<F>>{ { func }, { func, count }};
}

// ********************************** all ********************************************************

template<typename R> auto all(R&& range)
{
	return iterator_range<range_begin_type_t<R>>{begin(range), end(range)};
}

auto all()
{
	return make_pipeable([=](auto&& range) {return all(range); });
}

// ******************************************* transform ******************************************

template<typename R, typename F>
struct transform_iterator : iterator_facade<
	transform_iterator<R, F>,
	decltype(fake<F>()(fake<range_ref_type_t<R>>())),
	range_iter_categ_t<R>
>
{
	range_begin_type_t<R> iter_;
	F func_;
	
	transform_iterator(range_begin_type_t<R> const& iter, F const& func) : iter_(iter), func_(func) {}

	transform_iterator& operator=(transform_iterator const& ot)
	{
		iter_ = ot.iter_;
		//func_ = ot.func_;
		return *this;
	}

	void increment() { ++iter_; }
	void advance(intptr_t off) { iter_ += off; }
	void decrement() { --iter_; }
	auto distance_to(transform_iterator r) const { return iter_ - r.iter_; }
	auto dereference() const { return func_(*iter_); }
	bool equal(transform_iterator r) const { return iter_ == r.iter_; }
};

template<typename R, typename F> auto transform(R&& range, F&& func)
{
	using iterator = transform_iterator<std::remove_reference_t<R>, std::remove_reference_t<F>>;
	return iterator_range<iterator>{ { begin(range), func }, { end(range), func } };
}

template<typename F> auto transform(F&& func)
{
	return make_pipeable([=](auto&& range) {return transform(range, func); });
}

// ***************************************** slice ************************************************

template<typename R> auto slice(R&& range, size_t begin_idx, size_t end_idx)
{
	auto iter = begin(range);
	std::advance(iter, begin_idx);
	auto endIter = iter;
	std::advance(endIter, intptr_t(end_idx) - intptr_t(begin_idx));
	return iterator_range<decltype(iter)>{ {iter}, { endIter } };
}

auto slice(size_t begin, size_t end)
{
	return make_pipeable([=](auto&& range) {return slice(range, begin, end); });
}

// ***************************************** stride ***********************************************

template<typename R>
struct stride_iterator : iterator_facade<stride_iterator<R>, range_ref_type_t<R>, range_iter_categ_t<R>>
{
	range_begin_type_t<R> iter_;
	range_end_type_t<R> end_;
	size_t step_;
	
	stride_iterator(range_begin_type_t<R> const& iter, range_end_type_t<R> const& end, size_t step)
		: iter_(iter), end_(end), step_(step) {}

	auto increment()
	{
		for (size_t i = 0; i < step_ && iter_ != end_; ++i)
			++iter_;
	}

	auto decrement()
	{
		for (size_t i = 0; i < step_; ++i)
			--iter_;
	}

	void advance(intptr_t value) { iter_ += step_ * value; }
	auto dereference() const { return *iter_; }
	bool equal(stride_iterator other) const { return iter_ == other.iter_; }
	auto distance_to(stride_iterator other) const { return (iter_ - other.iter_) / step_; }
};


template<typename R> auto stride(R&& range, size_t step)
{
	auto iter = begin(range);
	auto endIter = end(range);
	return iterator_range<stride_iterator<std::remove_reference_t<R>>>{ 
		{ iter, endIter, step}, { endIter, endIter, step }};
}

auto stride(size_t step)
{
	return make_pipeable([=](auto&& range) {return stride(range, step); });
}

// ***************************************** retro ************************************************

template<typename R> auto retro(R&& range)
{
	return make_iterator_range(
		std::make_reverse_iterator(end(range)), std::make_reverse_iterator(begin(range)));
}

auto retro()
{
	return make_pipeable([=](auto&& range) {return retro(range); });
}

// *************************** zip *****************************************************
/// \cond PRIVATE 
namespace details
{
template <typename Tuple, typename F, std::size_t ...Indices>
void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
{
	using swallow = int[];
	(void)swallow {
		1,
			(f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...
	};
}

template <typename Tuple, typename F>
void for_each(Tuple&& tuple, F&& f)
{
	constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
	for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f),
		std::make_index_sequence<N>{});
}

template <class F, typename... Args, size_t... Is>
auto transform_each_impl(const std::tuple<Args...>& t, F&& f, std::index_sequence<Is...>) {
	return std::make_tuple(
		f(std::get<Is>(t))...
	);
}

template <class F, typename... Args>
auto transform_each(const std::tuple<Args...>& t, F&& f) {
	return transform_each_impl(
		t, std::forward<F>(f), std::make_index_sequence<sizeof...(Args)>{});
}

struct get_iter_value
{
	template<typename I>
	auto operator()(I&& iter)
	{
		return *std::forward<I>(iter);
	};
};

} // namespace details
/// \endcond

template<typename IterTuple>
struct zip_iterator : iterator_facade<
	zip_iterator<IterTuple>,
	decltype(transform_each(fake<IterTuple>(), details::get_iter_value())),
	std::bidirectional_iterator_tag
>
{
	IterTuple iters_;
	zip_iterator(IterTuple const& iters) : iters_(iters) {}
	void increment() { details::for_each(iters_, [](auto& iter) { ++iter; }); }
	void advance(intptr_t val) { for_each(iters_, [val](auto& iter) { iter += val; }); }
	void decrement() { details::for_each(iters_, [](auto& iter) { --iter; }); }
	auto dereference() const { return details::transform_each(iters_, details::get_iter_value()); }
	auto distance_to(zip_iterator other) const { return std::get<0>(iters_) - std::get<0>(other.iters_); }
	bool equal(zip_iterator other) const { return iters_ == other.iters_; }
};

template<typename ...R> auto zip(R&&... _ranges)
{
	auto iterTup = std::make_tuple(begin(std::forward<R>(_ranges))...);
	auto endTup = std::make_tuple(end(std::forward<R>(_ranges))...);
	return iterator_range<zip_iterator<decltype(iterTup)>>{ { iterTup }, { endTup }};
}

// ************************************ chunk *****************************************************

template<typename R>
struct chunk_iterator : iterator_facade<chunk_iterator<R>, iterator_range<range_begin_type_t<R>>, std::forward_iterator_tag>
{
	range_begin_type_t<R> iter_;
	range_begin_type_t<R> iter2_;
	range_end_type_t<R> end_;
	size_t step_;
	
	chunk_iterator(
		range_begin_type_t<R> const& iter, 
		range_begin_type_t<R> const& iter2, 
		range_end_type_t<R> const& end, 
		size_t step = 0)
		: iter_(iter), iter2_(iter2), end_(end), step_(step) 
	{
	}

	void increment()
	{
		iter_ = iter2_;
		for (size_t i = 0; i != step_ and iter2_ != end_; ++i)
			++iter2_;
	}

	auto dereference() const { return make_iterator_range(iter_, iter2_); }
	bool equal(chunk_iterator other) const { return iter_ == other.iter_; }
};

template<typename R> auto chunk(R&& range, size_t step)
{
	auto iter = begin(range);
	auto endIter = end(range);
	using iterator = chunk_iterator<std::remove_reference_t<R>>;
	iterator begin = { iter, iter, endIter, step };
	begin.increment();
	return iterator_range<iterator>{ { begin }, { endIter, endIter, endIter, step }};
}

auto chunk(size_t step)
{
	return make_pipeable([=](auto&& range) {return chunk(range, step); });
}

// ***************************************** filter ***********************************************

template<typename R, typename F>
struct filter_iterator : iterator_facade<filter_iterator<R, F>, range_ref_type_t<R>, range_iter_categ_t<R>>
{
	range_begin_type_t<R> begin_;
	range_begin_type_t<R> iter_;
	range_end_type_t<R> end_;
	F func_;
	
	filter_iterator(
		range_begin_type_t<R> const& begin, 
		range_begin_type_t<R> const& iter, 
		range_end_type_t<R> const& end, 
		F func)
		: begin_(begin), iter_(iter), end_(end), func_(func)
	{
	}

	void increment()
	{
		do
		{
			++iter_;
		} while (iter_ != end_ && not func_(*iter_));
	}

	void decrement()
	{
		do
		{
			--iter_;
		} while (not func_(*iter_) && iter_ != begin_);
	}

	auto dereference() const { return *iter_; }
	bool equal(filter_iterator other) const { return iter_ == other.iter_; }
};

template<typename R, typename F> auto filter(R&& range, F&& func)
{
	auto iter = begin(range);
	auto endIter = end(range);
	return iterator_range<filter_iterator<std::remove_reference_t<R>, std::remove_reference_t<F>>>{
		{ iter, iter, endIter, func },
		{ iter, endIter, endIter, func }
	};
}

template<typename F> auto filter(F&& func)
{
	return make_pipeable([=](auto&& range) {return filter(range, func); });
}

// ***************************************** join ***********************************************

template<typename IterPair, typename V>
struct join_iterator : iterator_facade<join_iterator<IterPair, V>, V, std::forward_iterator_tag>
{
	IterPair iter_;
	IterPair end_;
	size_t range_index_;
	
	join_iterator(IterPair const& iter, IterPair const& end, size_t range_index) 
		: iter_(iter), end_(end), range_index_(range_index) 
	{
	}

	void increment()
	{
		if (range_index_ == 0)
		{
			auto& i = std::get<0>(iter_);
			++i;
			if (i == std::get<0>(end_))
				range_index_ = 1;
		}
		else
			++std::get<1>(iter_);
	}

	auto dereference() const 
	{ 
		if (range_index_ == 0)
			return *std::get<0>(iter_);
		else
			return *std::get<1>(iter_);
	}

	bool equal(join_iterator other) const
	{ 
		if (range_index_ != other.range_index_)
			return false;
		if (range_index_ == 0)
			return std::get<0>(iter_) == std::get<0>(other.iter_);
		else
			return std::get<1>(iter_) == std::get<1>(other.iter_);
	}
};

template<typename R1, typename R2> auto join(R1&& range1, R2&& range2)
{
	return iterator_range<
		join_iterator<
		std::pair<range_begin_type_t<R1>, range_begin_type_t<R2>>,
		range_ref_type_t<R1>>>
	{
		{ std::make_pair(begin(range1), begin(range2)), std::make_pair(end(range1), end(range2)), 0 },
		{ std::make_pair(end(range1), end(range2)), std::make_pair(end(range1), end(range2)), 1 },
	};
}

template<typename R> auto join(R&& rightRange)
{
	auto rightRangeRef = make_iterator_range(begin(rightRange), end(rightRange));
	return make_pipeable([=](auto&& leftRange) {return join(leftRange, rightRangeRef); });
}

// *************************** enumerate **********************************************************

template<typename R> auto enumerate(R&& range)
{
	size_t const dist = std::distance(begin(std::forward<R>(range)), end(std::forward<R>(range)));
	return zip(iota(size_t(0), dist), std::forward<R>(range));
}

auto enumerate()
{
	return make_pipeable([=](auto&& range) {return enumerate(range); });
}

// ****************************** map_value ********************************************************

template<typename R> auto map_value(R&& range)
{
	return transform(std::forward<R>(range), [](auto nvp) {return std::get<1>(nvp); });
}

auto map_value()
{
	return make_pipeable([=](auto&& range) {return map_value(range); });
}

// ****************************** map_key **********************************************************

template<typename R> auto map_key(R&& range)
{
	return transform(std::forward<R>(range), [](auto nvp) {return std::get<0>(nvp); });
}

auto map_key()
{
	return make_pipeable([=](auto&& range) {return map_key(range); });
}

} // namespace view

// ****************************************** empty ***********************************************

/// @brief Check if the range if empty
///
/// @snippet rah.cpp rah::empty
template<typename R> bool empty(R&& range)
{
	return begin(range) == end(range);
}

/// @brief Check if the range if empty
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::empty_pipeable
auto empty()
{
	return make_pipeable([](auto&& range) {return empty(range); });
}

// ****************************************** transform *******************************************

/// @brief Applies the given function unary_op to the range rangeIn and stores the result in the range rangeOut
///
/// @snippet rah.cpp rah::transform3
template<typename RI, typename RO, typename F>
auto transform(RI&& rangeIn, RO&& rangeOut, F&& unary_op)
{
	auto iter = std::transform(begin(rangeIn), end(rangeIn), begin(rangeOut), std::forward<F>(unary_op));
	return make_iterator_range(iter, end(rangeOut));
}

/// @brief The binary operation binary_op is applied to pairs of elements from two ranges
///
/// @snippet rah.cpp rah::transform4
template<typename RI1, typename RI2, typename RO, typename F>
auto transform(RI1&& rangeIn1, RI2&& rangeIn2, RO&& rangeOut, F&& binary_op)
{
	auto iter = std::transform(
		begin(rangeIn1), end(rangeIn1),
		begin(rangeIn2),
		begin(rangeOut),
		std::forward<F>(binary_op));
	return make_iterator_range(iter, end(rangeOut));
}

// ********************************************* reduce *******************************************

/// @brief Executes a reducer function on each element of the range, resulting in a single output value
///
/// @snippet rah.cpp rah::reduce
template<typename R, typename I, typename F> auto reduce(R&& range, I&& init, F&& reducer)
{
	return std::accumulate(begin(range), end(range), std::forward<I>(init), std::forward<F>(reducer));
}

/// @brief Executes a reducer function on each element of the range, resulting in a single output value
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::reduce_pipeable
template<typename I, typename F>
auto reduce(I&& init, F&& reducer)
{
	return make_pipeable([=](auto&& range) {return reduce(range, init, reducer); });
}

// ************************* any_of *******************************************

/// @brief Checks if unary predicate pred returns true for at least one element in the range
///
/// @snippet rah.cpp rah::any_of
template<typename R, typename F> bool any_of(R&& range, F&& pred)
{
	return std::any_of(begin(range), end(range), std::forward<F>(pred));
}

/// @brief Checks if unary predicate pred returns true for at least one element in the range
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::any_of_pipeable
template<typename P> auto any_of(P&& pred)
{
	return make_pipeable([=](auto&& range) {return any_of(range, pred); });
}

// ************************* all_of *******************************************

/// @brief Checks if unary predicate pred returns true for all elements in the range
///
/// @snippet rah.cpp rah::all_of
template<typename R, typename P> bool all_of(R&& range, P&& pred)
{
	return std::all_of(begin(range), end(range), std::forward<P>(pred));
}

/// @brief Checks if unary predicate pred returns true for all elements in the range
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::all_of_pipeable
template<typename P> auto all_of(P&& pred)
{
	return make_pipeable([=](auto&& range) {return all_of(range, pred); });
}

// ************************* none_of *******************************************

/// @brief Checks if unary predicate pred returns true for no elements in the range 
///
/// @snippet rah.cpp rah::none_of
template<typename R, typename P> bool none_of(R&& range, P&& pred)
{
	return std::none_of(begin(range), end(range), std::forward<P>(pred));
}

/// @brief Checks if unary predicate pred returns true for no elements in the range 
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::none_of_pipeable
template<typename P> auto none_of(P&& pred)
{
	return make_pipeable([=](auto&& range) {return none_of(range, pred); });
}

// ************************* count ****************************************************************

/// @brief Counts the elements that are equal to value
///
/// @snippet rah.cpp rah::count
template<typename R, typename V> auto count(R&& range, V&& value)
{
	return std::count(begin(range), end(range), std::forward<V>(value));
}

/// @brief Counts the elements that are equal to value
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::count_pipeable
template<typename V> auto count(V&& value)
{
	return make_pipeable([=](auto&& range) {return count(range, value); });
}

/// @brief Counts elements for which predicate pred returns true
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::count_if
template<typename R, typename P> auto count_if(R&& range, P&& pred)
{
	return std::count_if(begin(range), end(range), std::forward<P>(pred));
}

/// @brief Counts elements for which predicate pred returns true
///
/// @snippet rah.cpp rah::count_if_pipeable
template<typename P> auto count_if(P&& pred)
{
	return make_pipeable([=](auto&& range) {return count_if(range, pred); });
}

// ************************* foreach **************************************************************

/// @brief Applies the given function func to each element of the range
///
/// @snippet rah.cpp rah::for_each
template<typename R, typename F> void for_each(R&& range, F&& func)
{
	::std::for_each(begin(range), end(range), std::forward<F>(func));
}

/// @brief Applies the given function func to each element of the range
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::for_each_pipeable
template<typename F> auto for_each(F&& func)
{
	return make_pipeable([=](auto&& range) {return for_each(range, func); });
}

// ***************************** to_container *****************************************************

/// @brief Return a container of type C, filled with the content of range
///
/// @snippet rah.cpp rah::to_container
template<typename C, typename R> auto to_container(R&& range)
{
	return C(begin(range), end(range));
}

/// @brief Return a container of type C, filled with the content of range
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::to_container_pipeable
template<typename C> auto to_container()
{
	return make_pipeable([=](auto&& range) {return to_container<C>(range); });
}

// ************************* mismatch *************************************************************

/// @brief Finds the first position where two ranges differ 
///
/// @snippet rah.cpp rah::mismatch
template<typename R1, typename R2> auto mismatch(R1&& range1, R2&& range2)
{
	auto const end1 = end(range1);
	auto const end2 = end(range2);
	auto i1_i2 = std::mismatch(begin(range1), end1, begin(range2), end2);
	return std::make_pair(make_iterator_range(std::get<0>(i1_i2), end1), make_iterator_range(std::get<1>(i1_i2), end2));
}

// ****************************************** find ************************************************

/// @brief Finds the first element equal to value
///
/// @snippet rah.cpp rah::find
template<typename R, typename V> auto find(R&& range, V&& value)
{
	auto end_iter = end(range);
	auto iter = std::find(begin(range), end_iter, std::forward<V>(value));
	return make_iterator_range(iter, end_iter);
}

/// @brief Finds the first element equal to value
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::find_pipeable
template<typename V> auto find(V&& value)
{
	return make_pipeable([=](auto&& range) {return find(range, value); });
}

/// @brief Finds the first element satisfying specific criteria
///
/// @snippet rah.cpp rah::find_if
template<typename R, typename P> auto find_if(R&& range, P&& pred)
{
	auto end_iter = end(range);
	auto iter = std::find_if(begin(range), end_iter, std::forward<P>(pred));
	return make_iterator_range(iter, end_iter);
}

/// @brief Finds the first element satisfying specific criteria
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::find_if_pipeable
template<typename P> auto find_if(P&& pred)
{
	return make_pipeable([=](auto&& range) {return find_if(range, pred); });
}

/// @brief Finds the first element not satisfying specific criteria
///
/// @snippet rah.cpp rah::find_if_not
template<typename R, typename P> auto find_if_not(R&& range, P&& pred)
{
	auto end_iter = end(range);
	auto iter = std::find_if_not(begin(range), end_iter, std::forward<P>(pred));
	return make_iterator_range(iter, end_iter);
}

/// @brief Finds the first element not satisfying specific criteria
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::find_if_not_pipeable
template<typename P> auto find_if_not(P&& pred)
{
	return make_pipeable([=](auto&& range) {return find_if_not(range, pred); });
}

// *************************************** copy ***************************************************

/// @brief Copy in range into an other
/// @return The part of out after the copied part
///
/// @snippet rah.cpp rah::copy
template<typename R1, typename R2> auto copy(R1&& in, R2&& out)
{
	auto iter = std::copy(begin(in), end(in), begin(out));
	return make_iterator_range(iter, end(out));
}

/// @brief Copy in range into an other
/// @return The part of out after the copied part
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::copy_into
template<typename R2> auto copy_into(R2&& out)
{
	auto all_out = out | rah::view::all();
	return make_pipeable([=](auto&& in) {return copy(in, all_out); });
}


// *************************************** size ***************************************************

/// @brief Get the size of range
///
/// @snippet rah.cpp rah::size
template<typename R> auto size(R&& range)
{
	return std::distance(begin(range), end(range));
}

/// @brief Get the size of range
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::size_pipeable
auto size()
{
	return make_pipeable([=](auto&& range) { return size(range); });
}

// *************************************** equal **************************************************

/// @brief Determines if two sets of elements are the same 
///
/// @snippet rah.cpp rah::equal
template<typename R1, typename R2> auto equal(R1&& range1, R2&& range2)
{
	return std::equal(begin(range1), end(range1), begin(range2), end(range2));
}

/// @brief Determines if two sets of elements are the same 
/// @remark pipeable syntax
///
/// @snippet rah.cpp rah::equal_pipeable
template<typename R1> auto equal(R1&& range2)
{
	auto all_range2 = range2 | rah::view::all();
	return make_pipeable([=](auto&& range1) { return equal(range1, all_range2); });
}

}

