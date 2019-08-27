//
// Copyright (c) 2019 Loïc HAMOT
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cassert>
#include <ciso646>

#ifndef RAH_DONT_USE_STD

#ifdef MSVC
#pragma warning(push, 0)
#endif
#include <type_traits>
#include <iterator>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <vector>
#include <array>
#ifdef MSVC
#pragma warning(pop)
#endif

#ifndef RAH_STD
#define RAH_STD std
#endif

#else

#include "rah_custom_std.hpp"

#endif

#ifndef RAH_NAMESPACE
#define RAH_NAMESPACE rah
#endif

namespace RAH_NAMESPACE
{

constexpr intptr_t End = -1; ///< Used with rah::view::slice to point to the end

// **************************** range traits ******************************************************

template<class T, size_t N> T* begin(T(&array)[N]) { return (T*)array; }

template<class T, size_t N> T* end(T(&array)[N]) noexcept { return array + N; }

/// Used in decltype to get an instance of a type
template<typename T> T& fake() { return *((RAH_STD::remove_reference_t<T>*)nullptr); }

template<typename T>
using range_begin_type_t = decltype(begin(fake<T>()));

template<typename T>
using range_end_type_t = decltype(end(fake<T>()));

template<typename T>
using range_ref_type_t = decltype(*begin(fake<T>()));

template<typename T>
using range_value_type_t = RAH_STD::remove_reference_t<range_ref_type_t<T>>;

template<typename R>
using range_iter_categ_t = typename RAH_STD::iterator_traits<range_begin_type_t<R>>::iterator_category;

template<typename R, typename = int>
struct is_range { static constexpr bool value = false; };

template<typename R>
struct is_range <R, decltype(begin(fake<R>()), end(fake<R>()), 0)> { static constexpr bool value = true; };

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
auto operator | (R&& range, pipeable<MakeRange> const& adapter) ->decltype(adapter.func(RAH_STD::forward<R>(range)))
{
	return adapter.func(RAH_STD::forward<R>(range));
}

// ************************************ iterator_facade *******************************************

namespace details
{

// Small optional impl for C++14 compilers
template<typename T> struct optional
{
	optional() = default;
	optional(optional const& other)
	{
		if (other.has_value())
		{
			new(getPtr()) T(other.get());
			is_allocated_ = true;
		}
	}
	optional& operator = (optional const& other)
	{
		if (has_value())
		{
			if (other.has_value())
			{
				// Handle the case where T is not copy assignable
				reset();
				new(getPtr()) T(other.get());
				is_allocated_ = true;
			}
			else
				reset();
		}
		else
		{
			if (other.has_value())
			{
				new(getPtr()) T(other.get());
				is_allocated_ = true;
			}
		}
		return *this;
	}
	optional& operator = (optional&& other)
	{
		if (has_value())
		{
			if (other.has_value())
			{
				// A lambda with const capture is not move assignable
				reset();
				new(getPtr()) T(RAH_STD::move(other.get()));
				is_allocated_ = true;
			}
			else
				reset();
		}
		else
		{
			if (other.has_value())
			{
				new(getPtr()) T(RAH_STD::move(other.get()));
				is_allocated_ = true;
			}
		}
		return *this;
	}
	optional(T const& other)
	{
		new(getPtr()) T(other);
		is_allocated_ = true;
	}
	optional& operator=(T const& other)
	{
		new(getPtr()) T(other);
		is_allocated_ = true;
		return *this;
	}
	optional& operator=(T&& other)
	{
		new(getPtr()) T(RAH_STD::move(other));
		is_allocated_ = true;
		return *this;
	}
	~optional() { reset(); }

	bool has_value() const { return is_allocated_; }

	void reset()
	{
		if (is_allocated_)
		{
			destruct_value();
			is_allocated_ = false;
		}
	}

	T& get() { assert(is_allocated_); return *getPtr(); }

	T const& get() const { assert(is_allocated_); return *getPtr(); }

	T& operator*() { return get(); }
	T const& operator*() const { return get(); }
	T* operator->() { assert(is_allocated_);  return getPtr(); }
	T const* operator->() const { assert(is_allocated_);  return getPtr(); }

private:
	T* getPtr() { return (T*)&value_; }
	T const* getPtr() const { return (T const*)&value_; }
	void destruct_value() { get().~T(); }

	RAH_STD::aligned_storage_t<sizeof(T), RAH_STD::alignment_of<T>::value> value_;
	bool is_allocated_ = false;
};
}

template<typename I, typename R, typename C> struct iterator_facade;

template<typename I, typename R>
struct iterator_facade<I, R, RAH_STD::forward_iterator_tag>
{
	template <class Reference>
	struct pointer_type
	{
		using type = RAH_NAMESPACE::details::optional<Reference>;
		template<typename Ref>
		static type to_pointer(Ref&& ref)
		{
			return RAH_STD::forward<Ref>(ref);
		}
	};

	template <class T>
	struct pointer_type<T&> // "real" references
	{
		using type = T*;
		template<typename Ref>
		static type to_pointer(Ref&& ref)
		{
			return &ref;
		}
	};

	using iterator_category = RAH_STD::forward_iterator_tag;
	using value_type = RAH_STD::remove_reference_t<R>;
	using difference_type = intptr_t;
	using pointer = typename pointer_type<R>::type;
	using reference = R;

	static_assert(not RAH_STD::is_reference<value_type>::value, "value_type can't be a reference");

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator++()
	{
		self().increment();
		return *this;
	}

	reference operator*() const 
	{ 
		static_assert(RAH_STD::is_same<decltype(self().dereference()), reference>::value, "");
		return self().dereference(); 
	}
	auto operator->() const { return pointer_type<R>::to_pointer(self().dereference()); }
	bool operator!=(I other) const { return not self().equal(other); }
	bool operator==(I other) const { return self().equal(other); }
};

template<typename I, typename R>
struct iterator_facade<I, R, RAH_STD::output_iterator_tag>
{
	using iterator_category = RAH_STD::forward_iterator_tag;
	using value_type = RAH_STD::remove_reference_t<R>;
	using difference_type = intptr_t;
	using pointer = value_type*;
	using reference = R;

	static_assert(not RAH_STD::is_reference<value_type>::value, "value_type can't be a reference");

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator++() { return *this; }
	auto& operator*() const { return *this; }
	bool operator!=(I other) const { return true; }
	bool operator==(I other) const { return false; }

	template<typename V>
	auto operator=(V&& value) const
	{
		self().put(RAH_STD::forward<V>(value));
		return self();
	}
};


template<typename I, typename R>
struct iterator_facade<I, R, RAH_STD::bidirectional_iterator_tag> : iterator_facade<I, R, RAH_STD::forward_iterator_tag>
{
	using iterator_category = RAH_STD::bidirectional_iterator_tag;

	I& self() { return *static_cast<I*>(this); }
	I const& self() const { return *static_cast<I const*>(this); }

	auto& operator--()
	{
		self().decrement();
		return *this;
	}
};

template<typename I, typename R>
struct iterator_facade<I, R, RAH_STD::random_access_iterator_tag> : iterator_facade<I, R, RAH_STD::bidirectional_iterator_tag>
{
	using iterator_category = RAH_STD::random_access_iterator_tag;

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
	bool operator<(I other) const { return self().distance_to(other) < 0; }
	bool operator<=(I other) const { return self().distance_to(other) <= 0; }
	bool operator>(I other) const { return self().distance_to(other) > 0; }
	bool operator>=(I other) const { return self().distance_to(other) >= 0; }
	auto operator[](intptr_t increment) const { return *(self() + increment); }
};

// ********************************** back_inserter ***********************************************

/// @see rah::back_inserter
template<typename C>
struct back_insert_iterator : iterator_facade<back_insert_iterator<C>, range_ref_type_t<C>, RAH_STD::output_iterator_tag>
{
	C* container_;
	back_insert_iterator(C& container) : container_(&container) { }
	template<typename V> void put(V&& value) const { container_->emplace_back(value); }
};

/// @brief Make a range which insert into the back of the a container
///
/// @snippet test.cpp rah::back_inserter
template<typename C> auto back_inserter(C&& container)
{
	using Container = RAH_STD::remove_reference_t<C>;
	auto begin = back_insert_iterator<Container>(container);
	auto end = back_insert_iterator<Container>(container);
	return RAH_NAMESPACE::make_iterator_range(begin, end);
}

/// Apply the '<' operator on two values of any type
struct is_lesser
{
	template<typename A, typename B> bool operator()(A&& a, B&& b) { return a < b; }
};

namespace view
{


// ********************************** all *********************************************************

template<typename R> auto all(R&& range)
{
	return iterator_range<range_begin_type_t<R>>{begin(range), end(range)};
}

inline auto all()
{
	return make_pipeable([=](auto&& range) {return all(range); });
}

// ******************************************* take ***********************************************

template<typename I>
struct take_iterator : iterator_facade<
	take_iterator<I>,
	decltype(*fake<I>()),
	typename RAH_STD::iterator_traits<I>::iterator_category
>
{
	I iter_;
	size_t count_ = size_t();

	take_iterator() = default;
	take_iterator(I iter, size_t count) : iter_(iter), count_(count) {}

	void increment() { ++iter_; ++count_; }
	void advance(intptr_t off) { iter_ += off; count_ += off; }
	void decrement() { --iter_; --count_; }
	auto distance_to(take_iterator r) const { return RAH_STD::min<intptr_t>(iter_ - r.iter_, count_ - r.count_); }
	auto dereference() const -> decltype(*iter_) { return *iter_; }
	bool equal(take_iterator r) const { return count_ == r.count_ || iter_ == r.iter_; }
};

template<typename R> auto take(R&& range, size_t count)
{
	using iterator = take_iterator<range_begin_type_t<R>>;
	iterator iter1(begin(range), 0);
	iterator iter2(end(range), count);
	return make_iterator_range(iter1, iter2);
}

inline auto take(size_t count)
{
	return make_pipeable([=](auto&& range) {return take(range, count); });
}

// ******************************************* counted ********************************************

template<typename I>
struct counted_iterator : iterator_facade<
	counted_iterator<I>,
	decltype(*fake<I>()),
	typename RAH_STD::iterator_traits<I>::iterator_category
>
{
	I iter_;
	size_t count_ = size_t();

	counted_iterator() = default;
	counted_iterator(I iter, size_t count) : iter_(iter), count_(count) {}

	void increment() { ++iter_; ++count_; }
	void advance(intptr_t off) { iter_ += off; count_ += off; }
	void decrement() { --iter_; --count_; }
	auto distance_to(counted_iterator r) const { return count_ - r.count_; }
	auto dereference() const -> decltype(*iter_) { return *iter_; }
	bool equal(counted_iterator r) const { return count_ == r.count_; }
};

template<typename I> auto counted(I&& it, size_t n, decltype(++it, 0) = 0)
{
	using iterator = counted_iterator<RAH_STD::remove_reference_t<I>>;
	iterator iter1(it, 0);
	iterator iter2(it, n);
	return make_iterator_range(iter1, iter2);
}

/// @cond
// Obsolete
template<typename R> auto counted(R&& range, size_t n, decltype(begin(range), 0) = 0)
{
	return take(range, n);
}

inline auto counted(size_t n)
{
	return make_pipeable([=](auto&& range) {return take(range, n); });
}
/// @endcond

// ******************************************* unbounded ******************************************

template<typename I>
struct unbounded_iterator : iterator_facade<
	unbounded_iterator<I>,
	decltype(*fake<I>()),
	typename RAH_STD::iterator_traits<I>::iterator_category
>
{
	I iter_;
	bool end_;

	unbounded_iterator() = default;
	unbounded_iterator(I iter, bool end) : iter_(iter), end_(end) {}

	void increment() { ++iter_; }

	void advance(intptr_t off) { iter_ += off; }
	void decrement() { --iter_; }
	auto distance_to(unbounded_iterator r) const
	{ 
		if (end_)
		{
			if (r.end_)
				return intptr_t{};
			else
				return RAH_STD::numeric_limits<intptr_t>::min();
		}
		else
		{
			if (r.end_)
				return RAH_STD::numeric_limits<intptr_t>::max();
			else
				return iter_ - r.iter_;
		}
	}

	auto dereference() const -> decltype(*iter_) { return *iter_; }
	bool equal(unbounded_iterator r) const 
	{ 
		return end_? 
			r.end_:
			(r.end_? false: r.iter_ == iter_);
	}
};

template<typename I> auto unbounded(I&& it)
{
	using iterator = unbounded_iterator<RAH_STD::remove_reference_t<I>>;
	iterator iter1(it, false);
	iterator iter2(it, true);
	return make_iterator_range(iter1, iter2);
}

// ********************************** ints ********************************************************

/// @see rah::ints
template<typename T = size_t>
struct ints_iterator : iterator_facade<ints_iterator<T>, T, RAH_STD::random_access_iterator_tag>
{
	T val_ = T();

	ints_iterator() = default;
	ints_iterator(T val) : val_(val) {}

	void increment() { ++val_; }
	void advance(intptr_t value) { val_ += T(value); }
	void decrement() { --val_; }
	auto distance_to(ints_iterator other) const { return (val_ - other.val_); }
	auto dereference() const { return val_; }
	bool equal(ints_iterator other) const { return val_ == other.val_; }
};

template<typename T = size_t> auto ints(T b = 0, T e = RAH_STD::numeric_limits<T>::max())
{
	return iterator_range<ints_iterator<T>>{ { b }, { e }};
}

// ********************************** iota ********************************************************

/// @see rah::iota
template<typename T = size_t>
struct iota_iterator : iterator_facade<iota_iterator<T>, T, RAH_STD::random_access_iterator_tag>
{
	T val_ = T();
	T step_ = T(1);

	iota_iterator() = default;
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
	auto diff = (e - b);
	diff = ((diff + (step - 1)) / step) * step;
	return iterator_range<iota_iterator<T>>{ { b, step}, { b + diff, step }};
}

// ********************************** repeat ******************************************************

/// @see rah::repeat
template<typename V>
struct repeat_iterator : iterator_facade<repeat_iterator<V>, V const&, RAH_STD::forward_iterator_tag>
{
	V val_ = V();

	repeat_iterator() = default;
	template<typename U>
	repeat_iterator(U val) : val_(RAH_STD::forward<U>(val)) {}

	void increment() { }
	void advance(intptr_t value) { }
	void decrement() { }
	V const& dereference() const { return val_; }
	bool equal(repeat_iterator) const { return false; }
};

template<typename V> auto repeat(V&& value)
{
	return iterator_range<repeat_iterator<RAH_STD::remove_const_t<RAH_STD::remove_reference_t<V>>>>{ { value}, { value }};
}

// ********************************** join ********************************************************

template<typename R>
struct join_iterator : iterator_facade<join_iterator<R>, range_ref_type_t<range_ref_type_t<R>>, RAH_STD::forward_iterator_tag>
{
	R range_;
	using Iterator1 = range_begin_type_t<R>;
	using Iterator2 = range_begin_type_t<decltype(*fake<Iterator1>())>;
	Iterator1 rangeIter_;
	struct SubRange
	{
		Iterator2 subRangeIter;
		Iterator2 subRangeEnd;
	};
	RAH_NAMESPACE::details::optional<SubRange> subRange_;

	template<typename U>
	join_iterator(U&& range, Iterator1 rangeIter, Iterator2 subRangeIter, Iterator2 subRangeEnd)
		: range_(RAH_STD::forward<U>(range))
		, rangeIter_(rangeIter)
		, subRange_(SubRange{ subRangeIter, subRangeEnd })
	{
		if (rangeIter_ == end(range_))
			return;
		next_valid();
	}

	template<typename U>
	join_iterator(U&& range, Iterator1 rangeIter)
		: range_(RAH_STD::forward<U>(range))
		, rangeIter_(rangeIter)
	{
	}

	void next_valid()
	{
		while (subRange_->subRangeIter == subRange_->subRangeEnd)
		{
			++rangeIter_;
			if (rangeIter_ == end(range_))
				return;
			else
			{
				subRange_->subRangeIter = begin(*rangeIter_);
				subRange_->subRangeEnd = end(*rangeIter_);
			}
		}
	}

	void increment()
	{
		++subRange_->subRangeIter;
		next_valid();
	}
	auto dereference() const ->decltype(*subRange_->subRangeIter) { return *subRange_->subRangeIter; }
	bool equal(join_iterator other) const
	{
		if (rangeIter_ == end(range_))
			return rangeIter_ == other.rangeIter_;
		else
			return rangeIter_ == other.rangeIter_ && subRange_->subRangeIter == other.subRange_->subRangeIter;
	}
};

template<typename R> auto join(R&& range_of_ranges)
{
	auto rangeRef = range_of_ranges | RAH_NAMESPACE::view::all();
	using join_iterator_type = join_iterator<decltype(rangeRef)>;
	auto rangeBegin = begin(rangeRef);
	auto rangeEnd = end(rangeRef);
	if (empty(rangeRef))
	{
		join_iterator_type b(rangeRef, rangeBegin);
		join_iterator_type e(rangeRef, rangeEnd);
		return make_iterator_range(b, e);
	}
	else
	{
		join_iterator_type b(rangeRef, rangeBegin, begin(*rangeBegin), end(*rangeBegin));
		join_iterator_type e(rangeRef, rangeEnd);
		return make_iterator_range(b, e);
	}
}

inline auto join()
{
	return make_pipeable([](auto&& range) {return join(range); });
}

// ********************************** cycle ********************************************************

template<typename R>
struct cycle_iterator : iterator_facade<cycle_iterator<R>, range_ref_type_t<R>, RAH_STD::forward_iterator_tag>
{
	R range_;
	using Iterator = range_begin_type_t<R>;
	Iterator beginIter_;
	Iterator endIter_;
	Iterator iter_;

	template<typename U>
	explicit cycle_iterator(U&& range, Iterator iter)
		: range_(RAH_STD::forward<U>(range))
		, beginIter_(begin(range_))
		, endIter_(end(range_))
		, iter_(iter)
	{
	}

	void increment()
	{
		++iter_;
		while (iter_ == endIter_)
			iter_ = begin(range_);
	}
	auto dereference() const ->decltype(*iter_) { return *iter_; }
	bool equal(cycle_iterator) const
	{
			return false;
	}
};

template<typename R> auto cycle(R&& range)
{
	auto rangeRef = range | RAH_NAMESPACE::view::all();
	using iterator_type = cycle_iterator<RAH_STD::remove_reference_t<decltype(rangeRef)>>;

	iterator_type b(rangeRef, begin(rangeRef));
	iterator_type e(rangeRef, begin(rangeRef));
	return make_iterator_range(b, e);
}

inline auto cycle()
{
	return make_pipeable([](auto&& range) {return cycle(range); });
}

// ********************************** generate ****************************************************

/// @see rah::generate
template<typename F>
struct generate_iterator : iterator_facade<generate_iterator<F>, decltype(fake<F>()()), RAH_STD::forward_iterator_tag>
{
	mutable RAH_NAMESPACE::details::optional<F> func_;

	generate_iterator(F const& func) : func_(func) {}

	void increment() { }
	auto dereference() const { return (*func_)(); }
	bool equal(generate_iterator) const { return false; }
};

template<typename F> auto generate(F&& func)
{
	using Functor = RAH_STD::remove_cv_t<RAH_STD::remove_reference_t<F>>;
	return iterator_range<generate_iterator<Functor>>{ { func}, { func }};
}

template<typename F> auto generate_n(size_t count, F&& func)
{
	return generate(RAH_STD::forward<F>(func)) | take(count);
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
	RAH_NAMESPACE::details::optional<F> func_;

	transform_iterator(range_begin_type_t<R> const& iter, F const& func) : iter_(iter), func_(func) {}

	transform_iterator& operator=(transform_iterator const& ot)
	{
		iter_ = ot.iter_;
		func_ = ot.func_;
		return *this;
	}

	void increment() { ++iter_; }
	void advance(intptr_t off) { iter_ += off; }
	void decrement() { --iter_; }
	auto distance_to(transform_iterator r) const { return iter_ - r.iter_; }
	auto dereference() const -> decltype((*func_)(*iter_)) { return (*func_)(*iter_); }
	bool equal(transform_iterator r) const { return iter_ == r.iter_; }
};

template<typename R, typename F> auto transform(R&& range, F&& func)
{
	using Functor = RAH_STD::remove_cv_t<RAH_STD::remove_reference_t<F>>;
	using iterator = transform_iterator<RAH_STD::remove_reference_t<R>, Functor>;
	auto iter1 = begin(range);
	auto iter2 = end(range);
	return iterator_range<iterator>{ { iter1, func }, { iter2, func } };
}

template<typename F> auto transform(F&& func)
{
	return make_pipeable([=](auto&& range) {return transform(range, func); });
}

// ********************************** for_each ****************************************************

template<typename R, typename F> auto for_each(R&& range, F&& func)
{
	return range | RAH_NAMESPACE::view::transform(func) | RAH_NAMESPACE::view::join();
}

template<typename F>
inline auto for_each(F&& func)
{
	return make_pipeable([=](auto&& range) {return RAH_NAMESPACE::view::for_each(range, func); });
}

// ***************************************** slice ************************************************

template<typename R> auto slice(R&& range, intptr_t begin_idx, intptr_t end_idx)
{
	static_assert(not RAH_STD::is_same<range_iter_categ_t<R>, RAH_STD::forward_iterator_tag>::value, 
		"Can't use slice on non-bidirectional iterators. Try to use view::drop and view::take");
	auto findIter = [](auto b, auto e, intptr_t idx)
	{
		if (idx < 0)
		{
			idx += 1;
			RAH_STD::advance(e, idx);
			return e;
		}
		else
		{
			RAH_STD::advance(b, idx);
			return b;
		}
	};
	auto b_in = begin(range);
	auto e_in = end(range);
	auto b_out = findIter(b_in, e_in, begin_idx);
	auto e_out = findIter(b_in, e_in, end_idx);
	return iterator_range<decltype(b_out)>{ {b_out}, { e_out } };
}

inline auto slice(intptr_t begin, intptr_t end)
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
	auto dereference() const -> decltype(*iter_) { return *iter_; }
	bool equal(stride_iterator other) const { return iter_ == other.iter_; }
	auto distance_to(stride_iterator other) const { return (iter_ - other.iter_) / step_; }
};


template<typename R> auto stride(R&& range, size_t step)
{
	auto iter = begin(range);
	auto endIter = end(range);
	return iterator_range<stride_iterator<RAH_STD::remove_reference_t<R>>>{
		{ iter, endIter, step}, { endIter, endIter, step }};
}

inline auto stride(size_t step)
{
	return make_pipeable([=](auto&& range) {return stride(range, step); });
}

// ***************************************** retro ************************************************

template<typename R> auto retro(R&& range)
{
	return make_iterator_range(
		RAH_STD::make_reverse_iterator(end(range)), RAH_STD::make_reverse_iterator(begin(range)));
}

inline auto retro()
{
	return make_pipeable([=](auto&& range) {return retro(range); });
}

// ********************************** single ******************************************************

template<typename V>
auto single(V&& value)
{
	return repeat(value) | take(1);
}

// *************************** zip ****************************************************************
/// \cond PRIVATE 
namespace details
{
template <typename Tuple, typename F, size_t ...Indices>
void for_each_impl(Tuple&& tuple, F&& f, RAH_STD::index_sequence<Indices...>)
{
	using swallow = int[];
	(void)swallow {
		1,
			(f(RAH_STD::get<Indices>(RAH_STD::forward<Tuple>(tuple))), void(), int{})...
	};
}

template <typename Tuple, typename F>
void for_each(Tuple&& tuple, F&& f)
{
	constexpr size_t N = RAH_STD::tuple_size<RAH_STD::remove_reference_t<Tuple>>::value;
	for_each_impl(RAH_STD::forward<Tuple>(tuple), RAH_STD::forward<F>(f),
		RAH_STD::make_index_sequence<N>{});
}

template <class F, typename... Args, size_t... Is>
auto transform_each_impl(const RAH_STD::tuple<Args...>& t, F&& f, RAH_STD::index_sequence<Is...>) {
	return RAH_STD::make_tuple(
		f(RAH_STD::get<Is>(t))...
	);
}

template <class F, typename... Args>
auto transform_each(const RAH_STD::tuple<Args...>& t, F&& f) {
	return transform_each_impl(
		t, RAH_STD::forward<F>(f), RAH_STD::make_index_sequence<sizeof...(Args)>{});
}

template <typename... Args, size_t... Is>
auto deref_impl(const RAH_STD::tuple<Args...>& t, RAH_STD::index_sequence<Is...>) {
	return RAH_STD::tuple<typename RAH_STD::iterator_traits<Args>::reference...>(
		(*RAH_STD::get<Is>(t))...
	);
}

template <typename... Args>
auto deref(const RAH_STD::tuple<Args...>& t) {
	return deref_impl(t, RAH_STD::make_index_sequence<sizeof...(Args)>{});
}

template <size_t Index>
struct Equal
{
	template <typename... Args>
	bool operator()(RAH_STD::tuple<Args...> const& a, RAH_STD::tuple<Args...> const& b) const
	{
		return (RAH_STD::get<Index - 1>(a) == RAH_STD::get<Index - 1>(b)) || Equal<Index - 1>{}(a, b);
	}
};

template<>
struct Equal<0>
{
	template <typename... Args>
	bool operator()(RAH_STD::tuple<Args...> const&, RAH_STD::tuple<Args...> const&) const
	{
		return false;
	}
};

template <typename... Args>
auto equal(RAH_STD::tuple<Args...> const& a, RAH_STD::tuple<Args...> const& b)
{
	return Equal<sizeof...(Args)>{}(a, b);
}

} // namespace details
/// \endcond

template<typename IterTuple>
struct zip_iterator : iterator_facade<
	zip_iterator<IterTuple>,
	decltype(details::deref(fake<IterTuple>())),
	RAH_STD::bidirectional_iterator_tag
>
{
	IterTuple iters_;
	zip_iterator() = default;
	zip_iterator(IterTuple const& iters) : iters_(iters) {}
	void increment() { details::for_each(iters_, [](auto& iter) { ++iter; }); }
	void advance(intptr_t val) { for_each(iters_, [val](auto& iter) { iter += val; }); }
	void decrement() { details::for_each(iters_, [](auto& iter) { --iter; }); }
	auto dereference() const { return details::deref(iters_); }
	auto distance_to(zip_iterator other) const { return RAH_STD::get<0>(iters_) - RAH_STD::get<0>(other.iters_); }
	bool equal(zip_iterator other) const { return details::equal(iters_, other.iters_); }
};

template<typename ...R> auto zip(R&&... _ranges)
{
	auto iterTup = RAH_STD::make_tuple(begin(RAH_STD::forward<R>(_ranges))...);
	auto endTup = RAH_STD::make_tuple(end(RAH_STD::forward<R>(_ranges))...);
	return iterator_range<zip_iterator<decltype(iterTup)>>{ { iterTup }, { endTup }};
}

// ************************************ chunk *****************************************************

template<typename R>
struct chunk_iterator : iterator_facade<chunk_iterator<R>, iterator_range<range_begin_type_t<R>>, RAH_STD::forward_iterator_tag>
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
	using iterator = chunk_iterator<RAH_STD::remove_reference_t<R>>;
	iterator begin = { iter, iter, endIter, step };
	begin.increment();
	return iterator_range<iterator>{ { begin }, { endIter, endIter, endIter, step }};
}

inline auto chunk(size_t step)
{
	return make_pipeable([=](auto&& range) {return chunk(range, step); });
}

// ***************************************** filter ***********************************************

template<typename R, typename F>
struct filter_iterator : iterator_facade<filter_iterator<R, F>, range_ref_type_t<R>, RAH_STD::bidirectional_iterator_tag>
{
	range_begin_type_t<R> begin_;
	range_begin_type_t<R> iter_;
	range_end_type_t<R> end_;
	RAH_NAMESPACE::details::optional<F> func_;
	decltype(iter_.operator->()) value_pointer_;

	filter_iterator(
		range_begin_type_t<R> const& begin,
		range_begin_type_t<R> const& iter,
		range_end_type_t<R> const& end,
		F func)
		: begin_(begin), iter_(iter), end_(end), func_(func)
	{
		next_value();
	}

	void next_value()
	{
		while (iter_ != end_ && not (*func_)(*(value_pointer_ = iter_.operator->())))
		{
			assert(iter_ != end_);
			++iter_;
		}
	}

	void increment()
	{
		++iter_;
		next_value();
	}

	void decrement()
	{
		do
		{
			--iter_;
		} while (not (*func_)(*iter_) && iter_ != begin_);
	}

	auto dereference() const -> decltype(*iter_) { return *value_pointer_; }
	bool equal(filter_iterator other) const { return iter_ == other.iter_; }
};

template<typename R, typename F> auto filter(R&& range, F&& func)
{
	auto iter = begin(range);
	auto endIter = end(range);
	using Predicate = RAH_STD::remove_cv_t<RAH_STD::remove_reference_t<F>>;
	return iterator_range<filter_iterator<RAH_STD::remove_reference_t<R>, Predicate>>{
		{ iter, iter, endIter, func },
		{ iter, endIter, endIter, func }
	};
}

template<typename F> auto filter(F&& func)
{
	return make_pipeable([=](auto&& range) {return filter(range, func); });
}

// ***************************************** concat ***********************************************

template<typename IterPair, typename V>
struct concat_iterator : iterator_facade<concat_iterator<IterPair, V>, V, RAH_STD::forward_iterator_tag>
{
	IterPair iter_;
	IterPair end_;
	size_t range_index_;

	concat_iterator(IterPair const& iter, IterPair const& end, size_t range_index)
		: iter_(iter), end_(end), range_index_(range_index)
	{
	}

	void increment()
	{
		if (range_index_ == 0)
		{
			auto& i = RAH_STD::get<0>(iter_);
			++i;
			if (i == RAH_STD::get<0>(end_))
				range_index_ = 1;
		}
		else
			++RAH_STD::get<1>(iter_);
	}

	auto dereference() const -> decltype(*RAH_STD::get<0>(iter_))
	{
		if (range_index_ == 0)
			return *RAH_STD::get<0>(iter_);
		else
			return *RAH_STD::get<1>(iter_);
	}

	bool equal(concat_iterator other) const
	{
		if (range_index_ != other.range_index_)
			return false;
		if (range_index_ == 0)
			return RAH_STD::get<0>(iter_) == RAH_STD::get<0>(other.iter_);
		else
			return RAH_STD::get<1>(iter_) == RAH_STD::get<1>(other.iter_);
	}
};

/// @brief return the same range
template<typename R1> auto concat(R1&& range1) 
{ 
	return RAH_STD::forward<R1>(range1); 
}

template<typename R1, typename R2> auto concat(R1&& range1, R2&& range2)
{
	auto begin_range1 = RAH_STD::make_pair(begin(range1), begin(range2));
	auto begin_range2 = RAH_STD::make_pair(end(range1), end(range2));
	auto end_range1 = RAH_STD::make_pair(end(range1), end(range2));
	auto end_range2 = RAH_STD::make_pair(end(range1), end(range2));
	return iterator_range<
		concat_iterator<
		RAH_STD::pair<range_begin_type_t<R1>, range_begin_type_t<R2>>,
		range_ref_type_t<R1>>>
	{
		{ begin_range1, begin_range2, 0 },
		{ end_range1, end_range2, 1 },
	};
}

/// @see rah::view::concat(R1&& range1, R2&& range2)
template<typename R1, typename R2, typename ...Ranges> 
auto concat(R1&& range1, R2&& range2, Ranges&&... ranges)
{
	return concat(concat(RAH_STD::forward<R1>(range1), RAH_STD::forward<R2>(range2)), ranges...);
}

// *************************** enumerate **********************************************************

template<typename R> auto enumerate(R&& range)
{
	size_t const dist = RAH_STD::distance(begin(RAH_STD::forward<R>(range)), end(RAH_STD::forward<R>(range)));
	return zip(iota(size_t(0), dist), RAH_STD::forward<R>(range));
}

inline auto enumerate()
{
	return make_pipeable([=](auto&& range) {return enumerate(range); });
}

// ****************************** map_value ********************************************************

template<typename R> auto map_value(R&& range)
{
	return transform(RAH_STD::forward<R>(range), [](auto&& nvp) -> decltype(RAH_STD::get<1>(RAH_STD::forward<decltype(nvp)>(nvp)))
	{
		return RAH_STD::get<1>(RAH_STD::forward<decltype(nvp)>(nvp)); 
	});
}

inline auto map_value()
{
	return make_pipeable([=](auto&& range) {return map_value(range); });
}

// ****************************** map_key **********************************************************

template<typename R> auto map_key(R&& range)
{
	return RAH_NAMESPACE::view::transform(RAH_STD::forward<R>(range), [](auto&& nvp) -> decltype(RAH_STD::get<0>(RAH_STD::forward<decltype(nvp)>(nvp)))
	{
		return RAH_STD::get<0>(RAH_STD::forward<decltype(nvp)>(nvp)); 
	});
}

inline auto map_key()
{
	return make_pipeable([=](auto&& range) {return map_key(range); });
}

// *********************************** sort *******************************************************

/// @brief Make a sorted view of a range
/// @return A view that is sorted
/// @remark This view is not lasy. The sorting is computed immediately.
///
/// @snippet test.cpp rah::view::sort
/// @snippet test.cpp rah::view::sort_pred
template<typename R, typename P = is_lesser, typename = RAH_STD::enable_if_t<is_range<R>::value>>
auto sort(R&& range, P&& pred = {})
{
	using value_type = range_value_type_t<R>;
	using Container = typename RAH_STD::vector<value_type>;
	Container result;
	result.reserve(RAH_STD::distance(begin(range), end(range)));
	RAH_STD::copy(begin(range), end(range), RAH_STD::back_inserter(result));
	RAH_STD::sort(begin(result), end(result), pred);
	return result;
}

/// @brief Make a sorted view of a range
/// @return A view that is sorted
/// @remark This view is not lasy. The sorting is computed immediately.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::view::sort_pipeable
/// @snippet test.cpp rah::view::sort_pred_pipeable
template<typename P = is_lesser, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto sort(P&& pred = {})
{
	return make_pipeable([=](auto&& range) { return view::sort(RAH_STD::forward<decltype(range)>(range), pred); });
}

} // namespace view

// ****************************************** empty ***********************************************

/// @brief Check if the range if empty
///
/// @snippet test.cpp rah::empty
template<typename R> bool empty(R&& range)
{
	return begin(range) == end(range);
}

/// @brief Check if the range if empty
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::empty_pipeable
inline auto empty()
{
	return make_pipeable([](auto&& range) {return empty(range); });
}

// ****************************************** equal_range ***********************************************

/// @brief Returns a range containing all elements equivalent to value in the range
///
/// @snippet test.cpp rah::equal_range
template<typename R, typename V>
auto equal_range(R&& range, V&& value, RAH_STD::enable_if_t<is_range<R>::value, int> = 0)
{
	auto pair = RAH_STD::equal_range(begin(range), end(range), RAH_STD::forward<V>(value));
	return make_iterator_range(RAH_STD::get<0>(pair), RAH_STD::get<1>(pair));
}

/// @brief Returns a range containing all elements equivalent to value in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::equal_range_pipeable
template<typename V> auto equal_range(V&& value)
{
	return make_pipeable([=](auto&& range) {return equal_range(RAH_STD::forward<decltype(range)>(range), value); });
}

/// @brief Returns a range containing all elements equivalent to value in the range
///
/// @snippet test.cpp rah::equal_range_pred_0
/// @snippet test.cpp rah::equal_range_pred
template<typename R, typename V, typename P>
auto equal_range(R&& range, V&& value, P&& pred)
{
	auto pair = RAH_STD::equal_range(begin(range), end(range), RAH_STD::forward<V>(value), RAH_STD::forward<P>(pred));
	return make_iterator_range(RAH_STD::get<0>(pair), RAH_STD::get<1>(pair));
}

/// @brief Returns a range containing all elements equivalent to value in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::equal_range_pred_0
/// @snippet test.cpp rah::equal_range_pred_pipeable
template<typename V, typename P>
auto equal_range(V&& value, P&& pred, RAH_STD::enable_if_t<!is_range<V>::value, int> = 0)
{
	return make_pipeable([=](auto&& range) {return equal_range(RAH_STD::forward<decltype(range)>(range), value, pred); });
}

// ****************************************** binary_search ***********************************************

/// @brief Checks if an element equivalent to value appears within the range
///
/// @snippet test.cpp rah::binary_search
template<typename R, typename V> auto binary_search(R&& range, V&& value)
{
	return RAH_STD::binary_search(begin(range), end(range), RAH_STD::forward<V>(value));
}

/// @brief Checks if an element equivalent to value appears within the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::binary_search_pipeable
template<typename V> auto binary_search(V&& value)
{
	return make_pipeable([=](auto&& range) {return binary_search(RAH_STD::forward<decltype(range)>(range), value); });
}

// ****************************************** transform *******************************************

/// @brief Applies the given function unary_op to the range rangeIn and stores the result in the range rangeOut
///
/// @snippet test.cpp rah::transform3
template<typename RI, typename RO, typename F>
auto transform(RI&& rangeIn, RO&& rangeOut, F&& unary_op)
{
	return RAH_STD::transform(begin(rangeIn), end(rangeIn), begin(rangeOut), RAH_STD::forward<F>(unary_op));
}

/// @brief The binary operation binary_op is applied to pairs of elements from two ranges
///
/// @snippet test.cpp rah::transform4
template<typename RI1, typename RI2, typename RO, typename F>
auto transform(RI1&& rangeIn1, RI2&& rangeIn2, RO&& rangeOut, F&& binary_op)
{
	return RAH_STD::transform(
		begin(rangeIn1), end(rangeIn1),
		begin(rangeIn2),
		begin(rangeOut),
		RAH_STD::forward<F>(binary_op));
}

// ********************************************* reduce *******************************************

/// @brief Executes a reducer function on each element of the range, resulting in a single output value
///
/// @snippet test.cpp rah::reduce
template<typename R, typename I, typename F> auto reduce(R&& range, I&& init, F&& reducer)
{
	return RAH_STD::accumulate(begin(range), end(range), RAH_STD::forward<I>(init), RAH_STD::forward<F>(reducer));
}

/// @brief Executes a reducer function on each element of the range, resulting in a single output value
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::reduce_pipeable
template<typename I, typename F>
auto reduce(I&& init, F&& reducer)
{
	return make_pipeable([=](auto&& range) {return reduce(range, init, reducer); });
}

// ************************* any_of *******************************************

/// @brief Checks if unary predicate pred returns true for at least one element in the range
///
/// @snippet test.cpp rah::any_of
template<typename R, typename F> bool any_of(R&& range, F&& pred)
{
	return RAH_STD::any_of(begin(range), end(range), RAH_STD::forward<F>(pred));
}

/// @brief Checks if unary predicate pred returns true for at least one element in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::any_of_pipeable
template<typename P> auto any_of(P&& pred)
{
	return make_pipeable([=](auto&& range) {return any_of(range, pred); });
}

// ************************* all_of *******************************************

/// @brief Checks if unary predicate pred returns true for all elements in the range
///
/// @snippet test.cpp rah::all_of
template<typename R, typename P> bool all_of(R&& range, P&& pred)
{
	return RAH_STD::all_of(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @brief Checks if unary predicate pred returns true for all elements in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::all_of_pipeable
template<typename P> auto all_of(P&& pred)
{
	return make_pipeable([=](auto&& range) {return all_of(range, pred); });
}

// ************************* none_of *******************************************

/// @brief Checks if unary predicate pred returns true for no elements in the range 
///
/// @snippet test.cpp rah::none_of
template<typename R, typename P> bool none_of(R&& range, P&& pred)
{
	return RAH_STD::none_of(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @brief Checks if unary predicate pred returns true for no elements in the range 
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::none_of_pipeable
template<typename P> auto none_of(P&& pred)
{
	return make_pipeable([=](auto&& range) {return none_of(range, pred); });
}

// ************************* count ****************************************************************

/// @brief Counts the elements that are equal to value
///
/// @snippet test.cpp rah::count
template<typename R, typename V> auto count(R&& range, V&& value)
{
	return RAH_STD::count(begin(range), end(range), RAH_STD::forward<V>(value));
}

/// @brief Counts the elements that are equal to value
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::count_pipeable
template<typename V> auto count(V&& value)
{
	return make_pipeable([=](auto&& range) {return count(range, value); });
}

/// @brief Counts elements for which predicate pred returns true
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::count_if
template<typename R, typename P> auto count_if(R&& range, P&& pred)
{
	return RAH_STD::count_if(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @brief Counts elements for which predicate pred returns true
///
/// @snippet test.cpp rah::count_if_pipeable
template<typename P> auto count_if(P&& pred)
{
	return make_pipeable([=](auto&& range) {return count_if(range, pred); });
}

// ************************* foreach **************************************************************

/// @brief Applies the given function func to each element of the range
///
/// @snippet test.cpp rah::for_each
template<typename R, typename F> auto for_each(R&& range, F&& func)
{
	return ::RAH_STD::for_each(begin(range), end(range), RAH_STD::forward<F>(func));
}

/// @brief Applies the given function func to each element of the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::for_each_pipeable
template<typename F> auto for_each(F&& func)
{
	return make_pipeable([=](auto&& range) {return for_each(range, func); });
}

// ***************************** to_container *****************************************************

/// @brief Return a container of type C, filled with the content of range
///
/// @snippet test.cpp rah::to_container
template<typename C, typename R> auto to_container(R&& range)
{
	return C(begin(range), end(range));
}

/// @brief Return a container of type C, filled with the content of range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::to_container_pipeable
template<typename C> auto to_container()
{
	return make_pipeable([=](auto&& range) {return to_container<C>(range); });
}

// ************************* mismatch *************************************************************

/// @brief Finds the first position where two ranges differ 
///
/// @snippet test.cpp rah::mismatch
template<typename R1, typename R2> auto mismatch(R1&& range1, R2&& range2)
{
	return RAH_STD::mismatch(begin(range1), end(range1), begin(range2), end(range2));
}

// ****************************************** find ************************************************

/// @brief Finds the first element equal to value
///
/// @snippet test.cpp rah::find
template<typename R, typename V> auto find(R&& range, V&& value)
{
	return RAH_STD::find(begin(range), end(range), RAH_STD::forward<V>(value));
}

/// @brief Finds the first element equal to value
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::find_pipeable
template<typename V> auto find(V&& value)
{
	return make_pipeable([=](auto&& range) {return find(range, value); });
}

/// @brief Finds the first element satisfying specific criteria
///
/// @snippet test.cpp rah::find_if
template<typename R, typename P> auto find_if(R&& range, P&& pred)
{
	return RAH_STD::find_if(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @brief Finds the first element satisfying specific criteria
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::find_if_pipeable
template<typename P> auto find_if(P&& pred)
{
	return make_pipeable([=](auto&& range) {return find_if(range, pred); });
}

/// @brief Finds the first element not satisfying specific criteria
///
/// @snippet test.cpp rah::find_if_not
template<typename R, typename P> auto find_if_not(R&& range, P&& pred)
{
	return RAH_STD::find_if_not(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @brief Finds the first element not satisfying specific criteria
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::find_if_not_pipeable
template<typename P> auto find_if_not(P&& pred)
{
	return make_pipeable([=](auto&& range) {return find_if_not(range, pred); });
}

// *************************************** copy ***************************************************

/// @brief Copy in range into an other
/// @return The part of out after the copied part
///
/// @snippet test.cpp rah::copy
template<typename R1, typename R2> auto copy(R1&& in, R2&& out)
{
	return RAH_STD::copy(begin(in), end(in), begin(out));
}

/// @brief Copy in range into an other
/// @return The part of out after the copied part
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::copy_pipeable
template<typename R2> auto copy(R2&& out)
{
	auto all_out = out | RAH_NAMESPACE::view::all();
	return make_pipeable([=](auto&& in) {return copy(in, all_out); });
}

// *************************************** fill ***************************************************

/// @brief Assigns the given value to the elements in the range [first, last)
///
/// @snippet test.cpp rah::copy
template<typename R1, typename V> auto fill(R1&& in, V&& value)
{
	return RAH_STD::fill(begin(in), end(in), value);
}

/// @brief Assigns the given value to the elements in the range [first, last)
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::copy_pipeable
template<typename V> auto fill(V&& value)
{
	return make_pipeable([=](auto&& out) {return fill(out, value); });
}

// *************************************** back_insert ***************************************************

/// @brief Insert *in* in back of *front*
///
/// @snippet test.cpp rah::back_insert
template<typename R1, typename R2> auto back_insert(R1&& in, R2&& out)
{
	return copy(in, RAH_NAMESPACE::back_inserter(out));
}

/// @brief Insert *in* in back of *front*
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::back_insert_pipeable
template<typename R2> auto back_insert(R2&& out)
{
	return make_pipeable([&](auto&& in) {return back_insert(in, out); });
}

// *************************************** copy_if ***************************************************

/// @brief Copies the elements for which the predicate pred returns true
/// @return The part of out after the copied part
///
/// @snippet test.cpp rah::copy_if
template<typename R1, typename R2, typename P> auto copy_if(R1&& in, R2&& out, P&& pred)
{
	return RAH_STD::copy_if(begin(in), end(in), begin(out), RAH_STD::forward<P>(pred));
}

/// @brief Copies the elements for which the predicate pred returns true
/// @return The part of out after the copied part
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::copy_if_pipeable
template<typename R2, typename P> auto copy_if(R2&& out, P&& pred)
{
	auto all_out = out | RAH_NAMESPACE::view::all();
	return make_pipeable([=](auto&& in) {return copy_if(in, all_out, pred); });
}

// *************************************** size ***************************************************

/// @brief Get the size of range
///
/// @snippet test.cpp rah::size
template<typename R> auto size(R&& range)
{
	return RAH_STD::distance(begin(range), end(range));
}

/// @brief Get the size of range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::size_pipeable
inline auto size()
{
	return make_pipeable([=](auto&& range) { return size(range); });
}

// *************************************** equal **************************************************

/// @brief Determines if two sets of elements are the same 
///
/// @snippet test.cpp rah::equal
template<typename R1, typename R2> auto equal(R1&& range1, R2&& range2)
{
#ifdef EASTL_VERSION
	return RAH_STD::identical(begin(range1), end(range1), begin(range2), end(range2));
#else
	return RAH_STD::equal(begin(range1), end(range1), begin(range2), end(range2));
#endif
}

/// @brief Determines if two sets of elements are the same 
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::equal_pipeable
template<typename R1> auto equal(R1&& range2)
{
	auto all_range2 = range2 | RAH_NAMESPACE::view::all();
	return make_pipeable([=](auto&& range1) { return equal(RAH_STD::forward<decltype(range1)>(range1), all_range2); });
}

// *********************************** stream_inserter ********************************************

template<typename S>
struct stream_inserter_iterator : iterator_facade<stream_inserter_iterator<S>, typename S::char_type, RAH_STD::output_iterator_tag>
{
	S* stream_;
	stream_inserter_iterator(S& stream) : stream_(&stream) { }
	template<typename V> void put(V&& value) const { (*stream_) << value; }
};

/// @brief Make a range which output to a stream
///
/// @snippet test.cpp rah::stream_inserter
template<typename S> auto stream_inserter(S&& stream)
{
	using Stream = RAH_STD::remove_reference_t<S>;
	auto begin = stream_inserter_iterator<Stream>(stream);
	auto end = stream_inserter_iterator<Stream>(stream);
	return RAH_NAMESPACE::make_iterator_range(begin, end);
}

// *********************************** remove_if **************************************************

/// @brief Keep at the begining of the range only elements for which pred(elt) is false\n
/// @return Return the (end) part of the range to erase.
///
/// @snippet test.cpp rah::remove_if
template<typename R, typename P> auto remove_if(R&& range, P&& pred)
{
	return RAH_STD::remove_if(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @brief Keep at the begining of the range only elements for which pred(elt) is false\n
/// @return The (end) part of the range to erase.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::remove_if_pipeable
template<typename P> auto remove_if(P&& pred)
{
	return make_pipeable([=](auto&& range) { return remove_if(range, pred); });
}

// *********************************** partition **************************************************

/// @brief Reorders the elements in the @b range in such a way that all elements for which the 
/// predicate @b pred returns `true` precede the elements for which predicate @b pred returns `false`. 
/// Relative order of the elements is not preserved. 
/// @return Iterator to the first element of the second group. 
///
/// @snippet test.cpp rah::partition
template<typename R, typename P> auto partition(R&& range, P&& pred)
{
	return RAH_STD::partition(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @see rah::partition(R&&, P&&)
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::partition_pipeable
template<typename P> auto partition(P&& pred)
{
	return make_pipeable([=](auto&& range) { return partition(range, pred); });
}

// *********************************** stable_partition *******************************************

/// @brief Reorders the elements in the @b range in such a way that all elements for which 
/// the predicate @b pred returns `true` precede the elements for which predicate @b pred returns false. 
/// Relative order of the elements is preserved. 
/// @return Iterator to the first element of the second group. 
///
/// @snippet test.cpp rah::stable_partition
template<typename R, typename P> auto stable_partition(R&& range, P&& pred)
{
	return RAH_STD::stable_partition(begin(range), end(range), RAH_STD::forward<P>(pred));
}

/// @see rah::stable_partition(R&&, P&&)
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::stable_partition_pipeable
template<typename P> auto stable_partition(P&& pred)
{
	return make_pipeable([=](auto&& range) { return stable_partition(range, pred); });
}

// *********************************** erase ******************************************************

/// @brief Erase a sub-range of a given container
/// @return A view on the resulting container
///
/// @snippet test.cpp rah::erase
template<typename C, typename R> auto erase(C&& container, R&& subrange)
{
	container.erase(begin(subrange), end(subrange));
	return container | RAH_NAMESPACE::view::all();
}

/// @brief Erase a sub-range of a given container
/// @return A view on the resulting container
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::erase_pipeable
template<typename R> auto erase(R&& range)
{
	auto all_range = range | RAH_NAMESPACE::view::all();
	return make_pipeable([=](auto&& cont) { return RAH_NAMESPACE::erase(cont, all_range); });
}

// *********************************** sort *******************************************************

/// @brief Sort a range in place, using the given predicate.
///
/// @snippet test.cpp rah::sort
/// @snippet test.cpp rah::sort_pred
template<typename R, typename P = is_lesser, typename = RAH_STD::enable_if_t<is_range<R>::value>>
void sort(R& range, P&& pred = {})
{
	RAH_STD::sort(begin(range), end(range), pred);
}

/// @brief Sort a range in place, using the given predicate.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::sort_pipeable
/// @snippet test.cpp rah::sort_pred_pipeable
template<typename P = is_lesser, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto sort(P&& pred = {})
{
	return make_pipeable([=](auto& range) { return sort(range, pred); });
}

// *********************************** stable_sort ************************************************

/// @brief Sorts the elements in the range in ascending order. The order of equivalent elements is guaranteed to be preserved. 
///
/// @snippet test.cpp rah::stable_sort
/// @snippet test.cpp rah::stable_sort_pred
template<typename R, typename P = is_lesser, typename = RAH_STD::enable_if_t<is_range<R>::value>>
void stable_sort(R& range, P&& pred = {})
{
	RAH_STD::stable_sort(begin(range), end(range), pred);
}

/// @brief Sorts the elements in the range in ascending order. The order of equivalent elements is guaranteed to be preserved. 
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::stable_sort_pipeable
/// @snippet test.cpp rah::stable_sort_pred_pipeable
template<typename P = is_lesser, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto stable_sort(P&& pred = {})
{
	return make_pipeable([=](auto& range) { return stable_sort(range, pred); });
}

// *********************************** shuffle *******************************************************

/// @brief Reorders the elements in the given range such that each possible permutation of those elements has equal probability of appearance. 
///
/// @snippet test.cpp rah::shuffle
template<typename R, typename URBG>
void shuffle(R& range, URBG&& g)
{
	RAH_STD::shuffle(begin(range), end(range), RAH_STD::forward<URBG>(g));
}

/// @brief Reorders the elements in the given range such that each possible permutation of those elements has equal probability of appearance. 
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::shuffle_pipeable
template<typename URBG>
auto shuffle(URBG&& g)
{
	return make_pipeable([&g](auto& range) { return RAH_NAMESPACE::shuffle(range, g); });
}

// *********************************** unique *****************************************************

/// Apply the '==' operator on two values of any type
struct is_equal
{
	template<typename A, typename B> bool operator()(A&& a, B&& b) { return a == b; }
};

/// @brief Remove all but first successuve values which are equals. Without resizing the range.
/// @return The end part of the range, which have to be remove.
///
/// @snippet test.cpp rah::unique
/// @snippet test.cpp rah::unique_pred
template<typename R, typename P = is_equal, typename = RAH_STD::enable_if_t<is_range<R>::value>>
auto unique(R&& range, P&& pred = {})
{
	return RAH_STD::unique(begin(range), end(range), pred);
}

/// @brief Remove all but first successuve values which are equals. Without resizing the range.
/// @return The end part of the range, which have to be remove.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::unique_pipeable
/// @snippet test.cpp rah::unique_pred_pipeable
template<typename P = is_equal, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto unique(P&& pred = {})
{
	return make_pipeable([=](auto&& range) { return unique(RAH_STD::forward<decltype(range)>(range), pred); });
}

// *********************************** set_difference ************************************************

/// @brief Copies the elements from the sorted range in1 which are not found in the sorted range in2 to the range out
/// The resulting range is also sorted.
///
/// @snippet test.cpp rah::set_difference
template<typename IN1, typename IN2, typename OUT_>
void set_difference(IN1&& in1, IN2&& in2, OUT_&& out)
{
	RAH_STD::set_difference(
		begin(in1), end(in1),
		begin(in2), end(in2),
		begin(out));
}

// *********************************** set_intersection ************************************************

/// @brief Copies the elements from the sorted range in1 which are also found in the sorted range in2 to the range out
/// The resulting range is also sorted.
///
/// @snippet test.cpp rah::set_intersection
template<typename IN1, typename IN2, typename OUT_>
void set_intersection(IN1&& in1, IN2&& in2, OUT_&& out)
{
	RAH_STD::set_intersection(
		begin(in1), end(in1),
		begin(in2), end(in2),
		begin(out));
}

namespace action
{

// *********************************** unique *****************************************************

/// @brief Remove all but first successive values which are equals.
/// @return Reference to container
///
/// @snippet test.cpp rah::action::unique
/// @snippet test.cpp rah::action::unique_pred
template<typename C, typename P = is_equal, typename = RAH_STD::enable_if_t<is_range<C>::value>>
auto&& unique(C&& container, P&& pred = {})
{
	container.erase(RAH_NAMESPACE::unique(container, pred), container.end());
	return RAH_STD::forward<C>(container);
}

/// @brief Remove all but first successuve values which are equals.
/// @return Reference to container
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::action::unique_pipeable
/// @snippet test.cpp rah::action::unique_pred_pipeable
template<typename P = is_equal, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto unique(P&& pred = {})
{
	return make_pipeable([=](auto&& range) -> auto&& { return action::unique(RAH_STD::forward<decltype(range)>(range), pred); });
}

// *********************************** remove_if **************************************************

/// @brief Keep only elements for which pred(elt) is false\n
/// @return reference to the container
///
/// @snippet test.cpp rah::action::remove_if
template<typename C, typename P> auto&& remove_if(C&& container, P&& pred)
{
	container.erase(RAH_NAMESPACE::remove_if(container, RAH_STD::forward<P>(pred)), container.end());
	return RAH_STD::forward<C>(container);
}

/// @brief Keep only elements for which pred(elt) is false\n
/// @return reference to the container
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::action::remove_if_pipeable
template<typename P> auto remove_if(P&& pred)
{
	return make_pipeable([=](auto&& range) -> auto&& { return remove_if(RAH_STD::forward<decltype(range)>(range), pred); });
}

// *********************************** sort *******************************************************

/// @brief Sort a range in place, using the given predicate.
/// @return reference to container
///
/// @snippet test.cpp rah::action::sort
/// @snippet test.cpp rah::action::sort_pred
template<typename C, typename P = is_lesser, typename = RAH_STD::enable_if_t<is_range<C>::value>>
auto&& sort(C&& container, P&& pred = {})
{
	RAH_NAMESPACE::sort(container, pred);
	return RAH_STD::forward<C>(container);
}

/// @brief Sort a range in place, using the given predicate.
/// @return reference to container
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::action::sort_pipeable
/// @snippet test.cpp rah::action::sort_pred_pipeable
template<typename P = is_lesser, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto sort(P&& pred = {})
{
	return make_pipeable([=](auto&& range) -> auto&& { return action::sort(RAH_STD::forward<decltype(range)>(range), pred); });
}

// *********************************** shuffle *******************************************************

/// @brief Reorders the elements in the given range such that each possible permutation of those elements has equal probability of appearance. 
/// @return reference to container
///
/// @snippet test.cpp rah::action::shuffle
template<typename C, typename URBG>
auto&& shuffle(C&& container, URBG&& g)
{
	URBG gen = RAH_STD::forward<URBG>(g);
	RAH_NAMESPACE::shuffle(container, gen);
	return RAH_STD::forward<C>(container);
}

/// @brief Reorders the elements in the given range such that each possible permutation of those elements has equal probability of appearance. 
/// @return reference to container
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::action::shuffle_pipeable
template<typename URBG>
auto shuffle(URBG&& g)
{
	return make_pipeable([&](auto&& range) -> auto&& { return action::shuffle(RAH_STD::forward<decltype(range)>(range), g); });
}

// *************************************** fill ***************************************************

/// @brief Assigns the given value to the elements in the range [first, last)
///
/// @snippet test.cpp rah::copy
template<typename R1, typename V> auto fill(R1&& in, V&& value)
{
	RAH_STD::fill(begin(in), end(in), value);
	return RAH_STD::forward<R1>(in);
}

/// @brief Assigns the given value to the elements in the range [first, last)
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::copy_pipeable
template<typename V> auto fill(V&& value)
{
	return make_pipeable([=](auto&& out) {return RAH_NAMESPACE::action::fill(out, value); });
}

}  // namespace action

}  // namespace rah
