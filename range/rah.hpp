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

template<typename T> T& fake()
{
	return *((T*)nullptr);
}

template<typename T>
using range_begin_type_t = decltype(std::begin(*((T*)nullptr)));

template<typename T>
using range_end_type_t = decltype(std::end(*((T*)nullptr)));

template<typename T>
using range_value_type_t = std::remove_reference_t<decltype(*std::begin(fake<T>()))>;

template<typename R>
using range_iter_categ_t = typename std::iterator_traits<decltype(std::begin(fake<R>()))>::iterator_category;

// ******************************** range *********************************************************

template<typename I>
struct iterator_range
{
	I beginIter;
	I endIter;

	I begin() const { return beginIter; }
	I begin() { return beginIter; }
	I end() const { return endIter; }
	I end() { return endIter; }
};

template<typename I>
auto make_iterator_range(I b, I e)
{
	return iterator_range<I>{b, e};
}

template<typename I> I begin(iterator_range<I>& r) { return r.beginIter; }
template<typename I> I end(iterator_range<I>& r) { return r.endIter; }
template<typename I> I begin(iterator_range<I> const& r) { return r.beginIter; }
template<typename I> I end(iterator_range<I> const& r) { return r.endIter; }

// ***************************************** adapter **********************************************

template<typename MakeRange>
struct Adapter
{
	MakeRange makeRange;
};

template<typename MakeRange>
auto MakeAdatper(MakeRange&& makeRange)
{
	return Adapter<MakeRange>{ makeRange };
}

template<typename R, typename MakeRange>
auto operator | (R&& range, Adapter<MakeRange> const& adapter)
{
	return adapter.makeRange(std::forward<R>(range));
}

// ************************************ IteratorHelper ********************************************

template<typename I, typename V>
struct ForwardIteratorHelper
{
	using iterator_category = std::forward_iterator_tag;
	using value_type = V;
	using difference_type = intptr_t;
	using pointer = V * ;
	using reference = V & ;

	static_assert(std::is_reference<value_type>::value == false, "value_type can't be a reference");

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator++()
	{
		self().incr();
		return *this;
	}

	auto operator*() const { return self().value(); }
	auto operator->() const { return &(self().value()); }
	bool operator!=(I other) const { return self().equal(other) == false; }
	bool operator==(I other) const { return self().equal(other); }
};

template<typename I, typename V>
struct BidirectionalIteratorHelper : ForwardIteratorHelper<I, V>
{
	using iterator_category = std::bidirectional_iterator_tag;

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator--()
	{
		self().decr();
		return *this;
	}
};

template<typename I, typename V>
struct RandomAccessIteratorHelper : BidirectionalIteratorHelper<I, V>
{
	using iterator_category = std::random_access_iterator_tag;

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator+=(intptr_t incr)
	{
		self().incr(incr);
		return *this;
	}

	auto operator+(intptr_t incr)
	{
		auto iter = self();
		iter.incr(incr);
		return iter;
	}

	auto& operator-=(intptr_t incr)
	{
		self().incr(-incr);
		return *this;
	}

	auto operator-(intptr_t incr)
	{
		auto iter = self();
		iter.incr(-incr);
		return iter;
	}

	auto operator-(I other) const { return self().sub(other); }
	bool operator<(I other) const { return sub(other) < 0; }
	bool operator<=(I other) const { return sub(other) <= 0; }
	bool operator>(I other) const { return sub(other) > 0; }
	bool operator>=(I other) const { return sub(other) >= 0; }
	auto operator[](intptr_t incr) const { return *(self() + incr); }
};

template<typename R, typename F, typename C>
struct IteratorHelper;

template<typename R, typename F>
struct IteratorHelper<R, F, std::random_access_iterator_tag>
{
	using Type = RandomAccessIteratorHelper<R, F>;
};

template<typename R, typename F>
struct IteratorHelper<R, F, std::bidirectional_iterator_tag>
{
	using Type = BidirectionalIteratorHelper<R, F>;
};

template<typename R, typename F>
struct IteratorHelper<R, F, std::forward_iterator_tag>
{
	using Type = ForwardIteratorHelper<R, F>;
};

namespace lazy
{

// ********************************** ints ********************************************************

template<typename T = size_t>
struct IntRangeIterator : IteratorHelper<IntRangeIterator<T>, T, std::random_access_iterator_tag>::Type
{
	T val;
	T step;

	void incr() { val += step; }
	void incr(intptr_t value) { val += T(step * value); }
	void decr() { val -= step; }
	auto sub(IntRangeIterator other) const { return (val - other.val) / step; }
	auto value() const { return val; }
	bool equal(IntRangeIterator other) const { return val == other.val; }
};

template<typename T = size_t> auto ints(T b, T e, T step = 1)
{
	assert(step != 0);
	return iterator_range<IntRangeIterator<T>>{ { {}, b, step}, { {}, e, step }};
}

// ********************************** generate ****************************************************

template<typename F>
struct GenerateIterator : IteratorHelper<GenerateIterator<F>, decltype(fake<F>()()), std::forward_iterator_tag>::Type
{
	mutable F func;
	size_t iterCount = 0;

	void incr() { ++iterCount; }
	auto value() const { return func(); }
	bool equal(GenerateIterator other) const { return iterCount == other.iterCount; }
};

template<typename F> auto generate(F&& func)
{
	return iterator_range<GenerateIterator<F>>{ { {}, func}, { {}, func }};
}

template<typename F> auto generate_n(F&& func, size_t count)
{
	return iterator_range<GenerateIterator<F>>{ { {}, func, 0}, { {}, func, count }};
}

// ******************************************* transform ******************************************

template<typename R, typename F>
struct TransformIterator : IteratorHelper<
	TransformIterator<R, F>,
	decltype(fake<F>()(fake<range_value_type_t<R>>())),
	range_iter_categ_t<R>
>::Type
{
	range_begin_type_t<R> iter;
	F func;

	TransformIterator& operator=(TransformIterator const& ot)
	{
		iter = ot.iter;
		//func = ot.func;
		return *this;
	}

	void incr() { ++iter; }
	void incr(intptr_t off) { iter += off; }
	void decr() { --iter; }
	auto sub(TransformIterator r) const { return iter - r.iter; }
	auto value() const { return func(*iter); }
	bool equal(TransformIterator r) const { return iter == r.iter; }
};

template<typename R, typename F> auto transform(R&& range, F&& func)
{
	using iterator = TransformIterator<std::remove_reference_t<R>, std::remove_reference_t<F>>;
	return iterator_range<iterator>{ { {}, std::begin(range), std::forward<F>(func)}, { {}, std::end(range), std::forward<F>(func) }};
}

template<typename F> auto transform(F&& func)
{
	return MakeAdatper([=](auto&& range) {return transform(range, func); });
}

// ***************************************** slice ********************************************************

template<typename R> auto slice(R&& range, size_t begin, size_t end)
{
	auto iter = std::begin(range);
	std::advance(iter, begin);
	auto endIter = iter;
	std::advance(endIter, intptr_t(end) - intptr_t(begin));
	return iterator_range<decltype(iter)>{ {iter}, { endIter } };
}

auto slice(size_t begin, size_t end)
{
	return MakeAdatper([=](auto&& range) {return slice(range, begin, end); });
}

// ***************************************** stride ***********************************************

template<typename R>
struct StrideIterator : IteratorHelper<StrideIterator<R>, range_value_type_t<R>, range_iter_categ_t<R>>::Type
{
	range_begin_type_t<R> iter;
	range_end_type_t<R> endIter;
	size_t step;

	auto incr()
	{
		for (size_t i = 0; i < step && iter != endIter; ++i)
			++iter;
	}

	auto decr()
	{
		for (size_t i = 0; i < step; ++i)
			--iter;
	}

	void incr(intptr_t value) { iter += step * value; }
	auto value() const { return *iter; }
	bool equal(StrideIterator other) const { return iter == other.iter; }
	auto sub(StrideIterator other) const { return (iter - other.iter) / step; }
};


template<typename R> auto stride(R&& range, size_t step)
{
	auto iter = std::begin(range);
	auto endIter = std::end(range);
	return iterator_range<StrideIterator<std::remove_reference_t<R>>>{ { {}, iter, endIter, step}, { {}, endIter, endIter, step }};
}

auto stride(size_t step)
{
	return MakeAdatper([=](auto&& range) {return stride(range, step); });
}

// ***************************************** retro ************************************************

template<typename R>
struct RetroIterator : IteratorHelper<RetroIterator<R>, range_value_type_t<R>, range_iter_categ_t<R>>::Type
{
	range_begin_type_t<R> iter;

	void incr() { --iter; }
	void decr() { ++iter; }
	void incr(intptr_t value) { iter -= value; }
	auto value() const
	{
		auto iter2 = iter;
		--iter2;
		return *iter2;
	}
	auto sub(RetroIterator other) const { return other.iter - iter; }
	bool equal(RetroIterator other) const { return iter == other.iter; }
};

template<typename R> auto retro(R&& range)
{
	return iterator_range<RetroIterator<std::remove_reference_t<R>>>({ {{}, std::end(range)}, {{}, std::begin(range)} });
}

auto retro()
{
	return MakeAdatper([=](auto&& range) {return retro(range); });
}

// *************************** zip *****************************************************
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

auto get_iter_value = [](auto&& iter) { return *iter; };

} // namespace details

template<typename IterTuple>
struct ZipRangeIterator : IteratorHelper<
	ZipRangeIterator<IterTuple>,
	decltype(transform_each(fake<IterTuple>(), details::get_iter_value)),
	std::bidirectional_iterator_tag
>::Type
{
	IterTuple iters;
	void incr() { details::for_each(iters, [](auto& iter) { ++iter; }); }
	void incr(intptr_t val) { for_each(iters, [val](auto& iter) { iter += val; }); }
	void decr() { details::for_each(iters, [](auto& iter) { --iter; }); }
	auto value() const { return details::transform_each(iters, details::get_iter_value); }
	auto sub(ZipRangeIterator other) const { return std::get<0>(iters) - std::get<0>(other.iters); }
	bool equal(ZipRangeIterator other) const { return iters == other.iters; }
};

template<typename ...R> auto zip(R&&... _ranges)
{
	auto iterTup = std::make_tuple(std::begin(std::forward<R>(_ranges))...);
	auto endTup = std::make_tuple(std::end(std::forward<R>(_ranges))...);
	return iterator_range<ZipRangeIterator<decltype(iterTup)>>{ { {}, iterTup }, { {}, endTup }};
}

// ************************************ chunk *****************************************************

template<typename R>
struct ChunkIterator : IteratorHelper<ChunkIterator<R>, iterator_range<range_begin_type_t<R>>, std::forward_iterator_tag>::Type
{
	range_begin_type_t<R> iter;
	range_begin_type_t<R> iter2;
	range_end_type_t<R> endIter;
	size_t step = 0;

	void incr()
	{
		iter = iter2;
		for (size_t i = 0; i != step and iter2 != endIter; ++i)
			++iter2;
	}

	auto value() const { return make_iterator_range(iter, iter2); }
	bool equal(ChunkIterator other) const { return iter == other.iter; }
};

template<typename R> auto chunk(R&& range, size_t step)
{
	auto iter = std::begin(range);
	auto endIter = std::end(range);
	using iterator = ChunkIterator<std::remove_reference_t<R>>;
	iterator begin = { {}, iter, iter, endIter, step };
	begin.incr();
	return iterator_range<iterator>{ { begin }, { {}, endIter, endIter, endIter, step }};
}

auto chunk(size_t step)
{
	return MakeAdatper([=](auto&& range) {return chunk(range, step); });
}

// ***************************************** filter ***********************************************

template<typename R, typename F>
struct FilterIterator : IteratorHelper<FilterIterator<R, F>, range_value_type_t<R>, range_iter_categ_t<R>>::Type
{
	range_begin_type_t<R> beginIter;
	range_begin_type_t<R> iter;
	range_end_type_t<R> endIter;
	F func;

	void incr()
	{
		do
		{
			++iter;
		} while (not func(*iter) && iter != endIter);
	}

	void decr()
	{
		do
		{
			--iter;
		} while (not func(*iter) && iter != beginIter);
	}

	auto value() const { return *iter; }
	bool equal(FilterIterator other) const { return iter == other.iter; }
};

template<typename R, typename F> auto filter(R&& range, F&& func)
{
	auto iter = std::begin(range);
	auto endIter = std::end(range);
	return iterator_range<FilterIterator<std::remove_reference_t<R>, std::remove_reference_t<F>>>{
		{ {}, iter, iter, endIter, func },
		{ {}, iter, endIter, endIter, func }
	};
}

template<typename F> auto filter(F&& func)
{
	return MakeAdatper([=](auto&& range) {return filter(range, func); });
}

// ***************************************** join ***********************************************

template<typename IterPair, typename V>
struct JoinIterator : IteratorHelper<JoinIterator<IterPair, V>, V, std::forward_iterator_tag>::Type
{
	IterPair iter;
	IterPair endIter;
	size_t rangeIndex;

	void incr()
	{
		if (rangeIndex == 0)
		{
			auto& i = std::get<0>(iter);
			++i;
			if (i == std::get<0>(endIter))
				rangeIndex = 1;
		}
		else
			++std::get<1>(iter);
	}

	auto value() const 
	{ 
		if (rangeIndex == 0)
			return *std::get<0>(iter);
		else
			return *std::get<1>(iter);
	}

	bool equal(JoinIterator other) const
	{ 
		if (rangeIndex != other.rangeIndex)
			return false;
		if (rangeIndex == 0)
			return std::get<0>(iter) == std::get<0>(other.iter);
		else
			return std::get<1>(iter) == std::get<1>(other.iter);
	}
};

template<typename R1, typename R2> auto join(R1&& range1, R2&& range2)
{
	return iterator_range<
		JoinIterator<
		std::pair<range_begin_type_t<std::remove_reference_t<R1>>,
		range_begin_type_t<std::remove_reference_t<R2>>>,
		range_value_type_t<R1>>>
	{
		{ {}, std::make_pair(std::begin(range1), std::begin(range2)), std::make_pair(std::end(range1), std::end(range2)), 0 },
		{ {}, std::make_pair(std::end(range1), std::end(range2)), std::make_pair(std::end(range1), std::end(range2)), 1 },
	};
}

template<typename R> auto join(R&& rightRange)
{
	auto rightRangeRef = make_iterator_range(std::begin(rightRange), std::end(rightRange));
	return MakeAdatper([=](auto&& leftRange) {return join(leftRange, rightRangeRef); });
}

// *************************** enumerate **********************************************************

template<typename R> auto enumerate(R&& range)
{
	size_t const dist = std::distance(std::begin(std::forward<R>(range)), std::end(std::forward<R>(range)));
	return zip(ints<size_t>(0, dist), std::forward<R>(range));
}

auto enumerate()
{
	return MakeAdatper([=](auto&& range) {return enumerate(range); });
}

// ****************************** mapValue ********************************************************

template<typename R> auto mapValue(R&& range)
{
	return transform(std::forward<R>(range), [](auto nvp) {return std::get<1>(nvp); });
}

auto mapValue()
{
	return MakeAdatper([=](auto&& range) {return mapValue(range); });
}

// ****************************** mapKey **********************************************************

template<typename R> auto mapKey(R&& range)
{
	return transform(std::forward<R>(range), [](auto nvp) {return std::get<0>(nvp); });
}

auto mapKey()
{
	return MakeAdatper([=](auto&& range) {return mapKey(range); });
}

} // namespace lazy

// ****************************************** transform *******************************************

template<typename RI, typename RO, typename F> auto transform(RI&& rangeIn, RO&& rangeOut, F&& func)
{
	auto iter = std::transform(std::begin(rangeIn), std::end(rangeIn), std::begin(rangeOut), std::forward<F>(func));
	return make_iterator_range(iter, std::end(rangeOut));
}

template<typename RI1, typename RI2, typename RO, typename F> auto transform(RI1&& rangeIn1, RI2&& rangeIn2, RO&& rangeOut, F&& func)
{
	auto iter = std::transform(
		std::begin(rangeIn1), std::end(rangeIn1),
		std::begin(rangeIn2), std::end(rangeIn2),
		std::begin(rangeOut),
		std::forward<F>(func));
	return make_iterator_range(iter, std::end(rangeOut));
}

// ********************************************* reduce *******************************************

template<typename R, typename I, typename F> auto reduce(R&& range, I&& init, F&& func)
{
	return std::accumulate(std::begin(range), std::end(range), std::forward<I>(init), std::forward<F>(func));
}

template<typename I, typename F>
auto reduce(I&& init, F&& func)
{
	return MakeAdatper([=](auto&& range) {return reduce(range, init, func); });
}

// ************************* any_of *******************************************

template<typename R, typename F> auto any_of(R&& range, F&& func)
{
	return std::any_of(std::begin(range), std::end(range), std::forward<F>(func));
}

template<typename P> auto any_of(P&& pred)
{
	return MakeAdatper([=](auto&& range) {return any_of(range, pred); });
}

// ************************* all_of *******************************************

template<typename R, typename P> auto all_of(R&& range, P&& pred)
{
	return std::all_of(std::begin(range), std::end(range), std::forward<P>(pred));
}

template<typename P> auto all_of(P&& pred)
{
	return MakeAdatper([=](auto&& range) {return all_of(range, pred); });
}

// ************************* none_of *******************************************

template<typename R, typename P> auto none_of(R&& range, P&& pred)
{
	return std::none_of(std::begin(range), std::end(range), std::forward<P>(pred));
}

template<typename P> auto none_of(P&& pred)
{
	return MakeAdatper([=](auto&& range) {return none_of(range, pred); });
}

// ************************* count ****************************************************************

template<typename R, typename V> auto count(R&& range, V&& value)
{
	return std::count(std::begin(range), std::end(range), std::forward<V>(value));
}

template<typename V> auto count(V&& value)
{
	return MakeAdatper([=](auto&& range) {return count(range, value); });
}

template<typename R, typename P> auto count_if(R&& range, P&& pred)
{
	return std::count_if(std::begin(range), std::end(range), std::forward<P>(pred));
}

template<typename P> auto count_if(P&& pred)
{
	return MakeAdatper([=](auto&& range) {return count_if(range, pred); });
}

// ************************* foreach **************************************************************

template<typename R, typename F> void for_each(R&& range, F&& func)
{
	::std::for_each(std::begin(range), std::end(range), std::forward<F>(func));
}

template<typename F> auto for_each(F&& func)
{
	return MakeAdatper([=](auto&& range) {return for_each(range, func); });
}

// ***************************** to_container *****************************************************

template<typename C, typename R> auto to_container(R&& range)
{
	return C(std::begin(range), std::end(range));
}

template<typename C> auto to_container()
{
	return MakeAdatper([=](auto&& range) {return to_container<C>(range); });
}

// ************************* mismatch ****************************************************************

template<typename R1, typename R2> auto mismatch(R1&& range1, R2&& range2)
{
	auto const end1 = std::end(range1);
	auto const end2 = std::end(range2);
	auto[iter1, iter2] = std::mismatch(std::begin(range1), end1, std::begin(range2), end2);
	return std::make_pair(make_iterator_range(iter1, end1), make_iterator_range(iter2, end2));
}

// ****************************************** find ************************************************

template<typename R, typename V> auto find(R&& range, V&& value)
{
	auto end = std::end(range);
	auto iter = std::find(std::begin(range), end, std::forward<V>(value));
	return make_iterator_range(iter, end);
}

template<typename V> auto find(V&& value)
{
	return MakeAdatper([=](auto&& range) {return find(range, value); });
}

template<typename R, typename P> auto find_if(R&& range, P&& pred)
{
	auto end = std::end(range);
	auto iter = std::find_if(std::begin(range), end, std::forward<P>(pred));
	return make_iterator_range(iter, end);
}

template<typename P> auto find_if(P&& pred)
{
	return MakeAdatper([=](auto&& range) {return find_if(range, pred); });
}

template<typename R, typename P> auto find_if_not(R&& range, P&& pred)
{
	auto end = std::end(range);
	auto iter = std::find_if_not(std::begin(range), end, std::forward<P>(pred));
	return make_iterator_range(iter, end);
}

template<typename P> auto find_if_not(P&& pred)
{
	return MakeAdatper([=](auto&& range) {return find_if_not(range, pred); });
}

// *************************************** size ***************************************************

template<typename R> auto size(R&& range)
{
	return std::distance(std::begin(range), std::end(range));
}

auto size()
{
	return MakeAdatper([=](auto&& range) { return size(range); });
}

// *************************************** equal ***************************************************

template<typename R1, typename R2> auto equal(R1&& range1, R2&& range2)
{
	return std::equal(std::begin(range1), std::end(range1), std::begin(range2), std::end(range2));
}

}

