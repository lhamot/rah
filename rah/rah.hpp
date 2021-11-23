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

namespace RAH_STD
{
	template<class T, std::size_t Extent> class span;
}

namespace RAH_NAMESPACE
{

constexpr intptr_t End = -1; ///< Used with rah::view::slice to point to the end

// **************************** range traits ******************************************************

template<class T, size_t N> T* begin(T(&array)[N]) { return (T*)array; }

template<class T, size_t N> T* end(T(&array)[N]) noexcept { return array + N; }

template<typename Container, typename Check = int>
struct has_member_begin_end
{
    static constexpr bool value = false;
};

template<typename Container>
struct has_member_begin_end<
    Container,
    decltype(
        std::declval<Container>().begin(),
        std::declval<Container>().end(),
        0)>
{
    static constexpr bool value = true;
};

template< class T >
constexpr bool has_member_begin_end_v = has_member_begin_end<T>::value;

template<typename Container, typename Check = int>
struct has_free_begin_end
{
	static constexpr bool value = false;
};

template<typename Container>
struct has_free_begin_end<
	Container,
	decltype(
		begin(std::declval<Container>()),
		end(std::declval<Container>()),
		0)>
{
	static constexpr bool value = true;
};

template< class T >
constexpr bool has_free_begin_end_v = has_free_begin_end<T>::value;

template<class T, size_t N> T* rah_begin(T(&array)[N]) { return (T*)array; }

template<class T, size_t N> T* rah_end(T(&array)[N]) noexcept { return array + N; }

/// Call the member begin if it exists
/// This avoid some ADL fail when using conteners with mixed namespaces
template<class Container, std::enable_if_t<has_member_begin_end_v<Container>, int> = 0>
auto rah_begin(Container&& container)
{
    return container.begin();
}

/// Call the member end if it exists
template<class Container, std::enable_if_t<has_member_begin_end_v<Container>, int> = 0>
auto rah_end(Container&& container)
{
    return container.end();
}

/// Call the free begin if there is no member begin
template<
	class Container,
	std::enable_if_t<has_free_begin_end_v<Container> and not has_member_begin_end_v<Container>, int> = 0>
auto rah_begin(Container&& container)
{
	return begin(container);
}

/// Call the free end if there is no member end
template<
	class Container,
	std::enable_if_t<has_free_begin_end_v<Container> and not has_member_begin_end_v<Container>, int> = 0>
auto rah_end(Container&& container)
{
	return end(container);
}

/// Used in decltype to get an instance of a type
template<typename T> T& fake() { return *((RAH_STD::remove_reference_t<T>*)nullptr); }

template<typename T>
using range_begin_type_t = decltype(rah_begin(fake<T>()));

template<typename T>
using range_end_type_t = decltype(rah_end(fake<T>()));

template<typename T>
using range_ref_type_t = decltype(*rah_begin(fake<T>()));

template<typename T>
using range_value_type_t = RAH_STD::remove_reference_t<range_ref_type_t<T>>;

template<typename R>
using range_iter_categ_t = typename RAH_STD::iterator_traits<range_begin_type_t<R>>::iterator_category;

template<typename R, typename = int>
struct is_range { static constexpr bool value = false; };

template<typename R>
struct is_range <R, decltype(rah_begin(fake<R>()), rah_end(fake<R>()), 0)> { static constexpr bool value = true; };

RAH_STD::output_iterator_tag get_common_iterator_tag(RAH_STD::output_iterator_tag, RAH_STD::output_iterator_tag);
RAH_STD::forward_iterator_tag get_common_iterator_tag(RAH_STD::forward_iterator_tag, RAH_STD::forward_iterator_tag);
RAH_STD::bidirectional_iterator_tag get_common_iterator_tag(RAH_STD::bidirectional_iterator_tag, RAH_STD::bidirectional_iterator_tag);
RAH_STD::random_access_iterator_tag get_common_iterator_tag(RAH_STD::random_access_iterator_tag, RAH_STD::random_access_iterator_tag);

template<typename A, typename B>
using common_iterator_tag = decltype(get_common_iterator_tag(A{}, B{}));

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
	optional& operator = (optional&& other) noexcept
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
		reset();
		new(getPtr()) T(other);
		is_allocated_ = true;
		return *this;
	}
	optional& operator=(T&& other)
	{
		reset();
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

	RAH_STD::aligned_storage_t<sizeof(T), RAH_STD::alignment_of<T>::value> value_{};
	bool is_allocated_ = false;
};
}

template<typename I, typename R, typename C> struct iterator_facade;

#define RAH_SELF (*static_cast<I*>(this))
#define RAH_SELF_CONST (*static_cast<I const*>(this))

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

	auto& operator++()
	{
		RAH_SELF.increment();
		return *this;
	}

	reference operator*() const 
	{ 
		static_assert(RAH_STD::is_same<decltype(RAH_SELF_CONST.dereference()), reference>::value, "");
		return RAH_SELF_CONST.dereference(); 
	}
	auto operator->() const { return pointer_type<R>::to_pointer(RAH_SELF_CONST.dereference()); }
	bool operator!=(I const& other) const { return not RAH_SELF_CONST.equal(other); }
	bool operator==(I const& other) const { return RAH_SELF_CONST.equal(other); }
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

	auto& operator++() { return *this; }
	auto& operator*() const { return *this; }
	bool operator!=(I const& other) const { return true; }
	bool operator==(I const& other) const { return false; }

	template<typename V>
	auto operator=(V&& value) const
	{
		RAH_SELF_CONST.put(RAH_STD::forward<V>(value));
		return RAH_SELF_CONST;
	}
};


template<typename I, typename R>
struct iterator_facade<I, R, RAH_STD::bidirectional_iterator_tag> : iterator_facade<I, R, RAH_STD::forward_iterator_tag>
{
	using iterator_category = RAH_STD::bidirectional_iterator_tag;

	auto& operator--()
	{
		RAH_SELF.decrement();
		return *this;
	}
};

template<typename I, typename R>
struct iterator_facade<I, R, RAH_STD::random_access_iterator_tag> : iterator_facade<I, R, RAH_STD::bidirectional_iterator_tag>
{
	using iterator_category = RAH_STD::random_access_iterator_tag;

	auto& operator+=(intptr_t increment)
	{
		RAH_SELF.advance(increment);
		return *this;
	}

	auto operator+(intptr_t increment)
	{
		auto iter = RAH_SELF;
		iter.advance(increment);
		return iter;
	}

	auto& operator-=(intptr_t increment)
	{
		RAH_SELF.advance(-increment);
		return *this;
	}

	auto operator-(intptr_t increment)
	{
		auto iter = RAH_SELF;
		iter.advance(-increment);
		return iter;
	}

	auto operator-(I const& other) const { return RAH_SELF_CONST.distance_to(other); }
	bool operator<(I const& other) const { return RAH_SELF_CONST.distance_to(other) < 0; }
	bool operator<=(I const& other) const { return RAH_SELF_CONST.distance_to(other) <= 0; }
	bool operator>(I const& other) const { return RAH_SELF_CONST.distance_to(other) > 0; }
	bool operator>=(I const& other) const { return RAH_SELF_CONST.distance_to(other) >= 0; }
	auto operator[](intptr_t increment) const { return *(RAH_SELF_CONST + increment); }
};

#undef RAH_SELF 


#ifdef RAH_DONT_USE_STD

// If rah is binded to an other standard library, learn to use std iterators anyway.

template<typename I, typename R>
struct iterator_facade<I, R, std::forward_iterator_tag> :
	iterator_facade<I, R, RAH_STD::forward_iterator_tag> {};

template<typename I, typename R>
struct iterator_facade<I, R, std::output_iterator_tag> :
	iterator_facade<I, R, RAH_STD::output_iterator_tag> {};

template<typename I, typename R>
struct iterator_facade<I, R, std::bidirectional_iterator_tag> :
	iterator_facade<I, R, RAH_STD::bidirectional_iterator_tag> {};

template<typename I, typename R>
struct iterator_facade<I, R, std::random_access_iterator_tag> :
	iterator_facade<I, R, RAH_STD::random_access_iterator_tag> {};

#endif

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

// ********************************** inserter ***********************************************

/// @see rah::back_inserter
template<typename C>
struct insert_iterator : iterator_facade<insert_iterator<C>, range_ref_type_t<C>, RAH_STD::output_iterator_tag>
{
	C* container_;
	using Iterator = RAH_NAMESPACE::range_begin_type_t<C>;
	Iterator iter_;
	template<typename I>
	insert_iterator(C& container, I&& iter) : container_(&container), iter_(RAH_STD::forward<I>(iter)){ }
	template<typename V> void put(V&& value) const { container_->insert(iter_, value); }
};

/// @brief Make a range which insert into the back of the a container
///
/// @snippet test.cpp rah::back_inserter
template<typename C, typename I> auto inserter(C&& container, I&& iter)
{
	using Container = RAH_STD::remove_reference_t<C>;
	auto begin = insert_iterator<Container>(container, RAH_STD::forward<I>(iter));
	auto end = insert_iterator<Container>(container, RAH_STD::forward<I>(iter));
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
	static_assert(not RAH_STD::is_rvalue_reference_v<R&&>, "Can't call 'all' on a rvalue container");
	return iterator_range<range_begin_type_t<R>>{rah_begin(range), rah_end(range)};
}

template<typename I> auto all(std::initializer_list<I> range)
{
	return iterator_range<decltype(rah_begin(range))>{rah_begin(range), rah_end(range)};
}

template<typename I, std::size_t E> auto all(RAH_STD::span<I, E>&& range)
{
	return iterator_range<decltype(rah_begin(range))>{rah_begin(range), rah_end(range)};
}

template<typename I> auto all(iterator_range<I>&& range) -> decltype(std::move(range))
{
	return std::move(range);
}

template<typename I> iterator_range<I> const& all(iterator_range<I> const& range)
{
	return range;
}

inline auto all()
{
	return make_pipeable([=](auto&& range)
		{
			return all(RAH_STD::forward<decltype(range)>(range));
		});
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
	auto distance_to(take_iterator const& r) const { return RAH_STD::min<intptr_t>(iter_ - r.iter_, count_ - r.count_); }
	auto dereference() const -> decltype(*iter_) { return *iter_; }
	bool equal(take_iterator const& r) const { return count_ == r.count_ || iter_ == r.iter_; }
};

template<typename R> auto take(R&& range, size_t count)
{
	using iterator = take_iterator<range_begin_type_t<R>>;
	auto view = all(RAH_STD::forward<R>(range));
	iterator iter1(rah_begin(view), 0);
	iterator iter2(rah_end(view), count);
	return make_iterator_range(iter1, iter2);
}

inline auto take(size_t count)
{
	return make_pipeable([=](auto&& range)
		{
			return take(RAH_STD::forward<decltype(range)>(range), count);
		});
}

// ******************************************* sliding ********************************************

template<typename I>
struct sliding_iterator : iterator_facade<
	sliding_iterator<I>,
	iterator_range<I>,
	typename RAH_STD::iterator_traits<I>::iterator_category
>
{
	// Actually store a closed range [begin, last]
	//   to avoid to exceed the end iterator of the underlying range
	I subRangeBegin_;
	I subRangeLast_;

	sliding_iterator() = default;
	sliding_iterator(I subRangeBegin, I subRangeLast)
		: subRangeBegin_(subRangeBegin)
		, subRangeLast_(subRangeLast)
	{
	}

	void increment() { ++subRangeBegin_; ++subRangeLast_; }
	void advance(intptr_t off) { subRangeBegin_ += off; subRangeLast_ += off; }
	void decrement() { --subRangeBegin_; --subRangeLast_; }
	auto distance_to(sliding_iterator const& r) const { return subRangeBegin_ - r.subRangeBegin_; }
	auto dereference() const
	{
		I endIter = subRangeLast_;
		++endIter;
		return make_iterator_range(subRangeBegin_, endIter);
	}
	bool equal(sliding_iterator const& r) const { return subRangeBegin_ == r.subRangeBegin_; }
};

template<typename R> auto sliding(R&& range, size_t n)
{
	size_t const closedSubRangeSize = n - 1;
	auto view = all(RAH_STD::forward<R>(range));
	auto const rangeEnd = rah_end(view);
	using iterator = sliding_iterator<range_begin_type_t<R>>;
	auto subRangeBegin = rah_begin(view);
	auto subRangeLast = subRangeBegin;
	for (size_t i = 0; i != closedSubRangeSize; ++i)
	{
		if (subRangeLast == rangeEnd)
			return make_iterator_range(iterator(rangeEnd, rangeEnd), iterator(rangeEnd, rangeEnd));
		++subRangeLast;
	}

	auto endSubRangeBegin = rangeEnd;
	RAH_STD::advance(endSubRangeBegin, -intptr_t(n - 1));

	iterator iter1(subRangeBegin, subRangeLast);
	iterator iter2(endSubRangeBegin, rangeEnd);
	return make_iterator_range(iter1, iter2);
}

inline auto sliding(size_t n)
{
	return make_pipeable([=](auto&& range)
		{
			return sliding(RAH_STD::forward<decltype(range)>(range), n);
		});
}

// ******************************************* drop_exactly ***************************************

template<typename R> auto drop_exactly(R&& range, size_t count)
{
	auto view = all(RAH_STD::forward<R>(range));
	auto iter1 = rah_begin(view);
	auto iter2 = rah_end(view);
	RAH_STD::advance(iter1, count);
	return make_iterator_range(iter1, iter2);
}

inline auto drop_exactly(size_t count)
{
	return make_pipeable([=](auto&& range)
		{
			return drop_exactly(RAH_STD::forward<decltype(range)>(range), count);
		});
}

// ******************************************* drop ***********************************************

template<typename R> auto drop(R&& range, size_t count)
{
	auto view = all(RAH_STD::forward<R>(range));
	auto iter1 = rah_begin(view);
	auto iter2 = rah_end(view);
	for(size_t i = 0; i < count; ++i)
	{
		if (iter1 == iter2)
			break;
		++iter1;
	}
	return make_iterator_range(iter1, iter2);
}

inline auto drop(size_t count)
{
	return make_pipeable([=](auto&& range)
		{
			return drop(RAH_STD::forward<decltype(range)>(range), count);
		});
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
	auto distance_to(counted_iterator const& r) const { return count_ - r.count_; }
	auto dereference() const -> decltype(*iter_) { return *iter_; }
	bool equal(counted_iterator const& r) const { return count_ == r.count_; }
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
template<typename R> auto counted(R&& range, size_t n, decltype(rah_begin(range), 0) = 0)
{
	return take(range, n);
}

inline auto counted(size_t n)
{
	return make_pipeable([=](auto&& range)
		{
			return take(RAH_STD::forward<decltype(range)>(range), n);
		});
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
	auto distance_to(unbounded_iterator const& r) const
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
	bool equal(unbounded_iterator const& r) const
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
	auto distance_to(ints_iterator const& other) const { return (val_ - other.val_); }
	auto dereference() const { return val_; }
	bool equal(ints_iterator const& other) const { return val_ == other.val_; }
};

template<typename T = size_t> auto ints(T b = 0, T e = RAH_STD::numeric_limits<T>::max())
{
	return iterator_range<ints_iterator<T>>{ b, e};
}

template<typename T = size_t> auto closed_ints(T b = 0, T e = RAH_STD::numeric_limits<T>::max() - 1)
{
	return iterator_range<ints_iterator<T>>{ b, e + 1};
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
	auto distance_to(iota_iterator const& other) const { return (val_ - other.val_) / step_; }
	auto dereference() const { return val_; }
	bool equal(iota_iterator const& other) const { return val_ == other.val_; }
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
	bool equal(repeat_iterator const&) const { return false; }
};

template<typename V> auto repeat(V&& value)
{
	return iterator_range<repeat_iterator<RAH_STD::remove_const_t<RAH_STD::remove_reference_t<V>>>>{ { value}, { value }};
}

// ********************************** join ********************************************************

template<typename R>
struct join_iterator : iterator_facade<join_iterator<R>, range_ref_type_t<range_ref_type_t<R>>, RAH_STD::forward_iterator_tag>
{
	using Iterator1 = range_begin_type_t<R>;
	using SubRangeType = RAH_STD::remove_reference_t<decltype(all(*fake<Iterator1>()))>;
	using Iterator2 = range_begin_type_t<decltype(*fake<Iterator1>())>;
	Iterator1 rangeIter_;
	Iterator1 rangeEnd_;
	Iterator2 subRangeIter;
	Iterator2 subRangeEnd;

	join_iterator() = default;

	template<typename SR>
	join_iterator(Iterator1 rangeIter, Iterator1 rangeEnd, SR&& subRange)
		: rangeIter_(rangeIter)
		, rangeEnd_(rangeEnd)
		, subRangeIter(rah_begin(subRange))
		, subRangeEnd(rah_end(subRange))
	{
		if (rangeIter_ == rangeEnd_)
			return;
		next_valid();
	}

	join_iterator(Iterator1 rangeIter, Iterator1 rangeEnd)
		: rangeIter_(rangeIter)
		, rangeEnd_(rangeEnd)
	{
	}

	void next_valid()
	{
		while (subRangeIter == subRangeEnd)
		{
			++rangeIter_;
			if (rangeIter_ == rangeEnd_)
				return;
			else
			{
				auto view = all(*rangeIter_);
				subRangeIter = rah_begin(view);
				subRangeEnd = rah_end(view);
			}
		}
	}

	void increment()
	{
		++subRangeIter;
		next_valid();
	}
	auto dereference() const ->decltype(*subRangeIter) { return *subRangeIter; }
	bool equal(join_iterator const& other) const
	{
		if (rangeIter_ == rangeEnd_)
			return rangeIter_ == other.rangeIter_;
		else
			return rangeIter_ == other.rangeIter_ && subRangeIter == other.subRangeIter;
	}
};

template<typename R> auto join(R&& range_of_ranges)
{
	auto rangeRef = RAH_NAMESPACE::view::all(range_of_ranges);
	using join_iterator_type = join_iterator<decltype(rangeRef)>;
	auto rangeBegin = rah_begin(rangeRef);
	auto rangeEnd = rah_end(rangeRef);
	if (empty(rangeRef))
	{
		join_iterator_type b(rangeBegin, rangeEnd);
		join_iterator_type e(rangeEnd, rangeEnd);
		return make_iterator_range(b, e);
	}
	else
	{
		auto firstSubRange = all(*rangeBegin);
		join_iterator_type b(
			rangeBegin,
			rangeEnd,
			RAH_STD::forward<decltype(firstSubRange)>(firstSubRange));
		join_iterator_type e(rangeEnd, rangeEnd);
		return make_iterator_range(b, e);
	}
}

inline auto join()
{
	return make_pipeable([](auto&& range) {return join(range); });
}

// ********************************** cycle ********************************************************

template<typename R>
struct cycle_iterator : iterator_facade<
	cycle_iterator<R>,
	range_ref_type_t<R>,
	common_iterator_tag<RAH_STD::bidirectional_iterator_tag, range_iter_categ_t<R>>>
{
	R range_;
	using Iterator = range_begin_type_t<R>;
	Iterator beginIter_;
	Iterator endIter_;
	Iterator iter_;
	int64_t cycleIndex_;

	cycle_iterator() = default;
	template<typename U>
	explicit cycle_iterator(U&& range, Iterator iter, int64_t cycleIndex)
		: range_(RAH_STD::forward<U>(range))
		, beginIter_(rah_begin(range_))
		, endIter_(rah_end(range_))
		, iter_(iter)
		, cycleIndex_(cycleIndex)
	{
	}

	void increment()
	{
		++iter_;
		while (iter_ == endIter_)
		{
			iter_ = rah_begin(range_);
			++cycleIndex_;
		}
	}
	void decrement()
	{
		while (iter_ == beginIter_)
		{
			iter_ = rah_end(range_);
			--cycleIndex_;
		}
		--iter_;
	}
	auto dereference() const ->decltype(*iter_) { return *iter_; }
	bool equal(cycle_iterator const& other) const
	{
		return cycleIndex_ == other.cycleIndex_ and iter_ == other.iter_;
	}
};

template<typename R> auto cycle(R&& range)
{
	auto rangeRef = range | RAH_NAMESPACE::view::all();
	using iterator_type = cycle_iterator<RAH_STD::remove_reference_t<decltype(rangeRef)>>;
	auto view = all(RAH_STD::forward<R>(range));
	iterator_type beginIter(rangeRef, rah_begin(view), 0);
	iterator_type endIter(rangeRef, rah_end(view), -1);
	return make_iterator_range(beginIter, endIter);
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

	generate_iterator() = default;
	generate_iterator(F const& func) : func_(func) {}

	void increment() { }
	auto dereference() const { return (*func_)(); }
	bool equal(generate_iterator const&) const { return false; }
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

	transform_iterator() = default;
	transform_iterator(range_begin_type_t<R> const& iter, F const& func) : iter_(iter), func_(func) {}

	void increment() { ++iter_; }
	void advance(intptr_t off) { iter_ += off; }
	void decrement() { --iter_; }
	auto distance_to(transform_iterator const& r) const { return iter_ - r.iter_; }
	auto dereference() const -> decltype((*func_)(*iter_)) { return (*func_)(*iter_); }
	bool equal(transform_iterator const& r) const { return iter_ == r.iter_; }
};

template<typename R, typename F> auto transform(R&& range, F&& func)
{
	using Functor = RAH_STD::remove_cv_t<RAH_STD::remove_reference_t<F>>;
	using iterator = transform_iterator<RAH_STD::remove_reference_t<R>, Functor>;
	auto view = all(RAH_STD::forward<R>(range));
	auto iter1 = rah_begin(view);
	auto iter2 = rah_end(view);
	return iterator_range<iterator>{ { iter1, func }, { iter2, func } };
}

template<typename F> auto transform(F&& func)
{
	return make_pipeable([=](auto&& range)
		{
			return transform(RAH_STD::forward<decltype(range)>(range), func);
		});
}

// ******************************************* set_difference *************************************

template<typename InputIt1, typename InputIt2>
struct set_difference_iterator : iterator_facade<
	set_difference_iterator<InputIt1, InputIt2>,
	typename RAH_STD::iterator_traits<InputIt1>::reference,
	RAH_STD::forward_iterator_tag
>
{
	InputIt1 first1_;
	InputIt1 last1_;
	InputIt2 first2_;
	InputIt2 last2_;

	set_difference_iterator() = default;
	set_difference_iterator(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
		: first1_(first1) , last1_(last1) , first2_(first2), last2_(last2)
	{
		next_value();
	}

	void next_value()
	{
		while (first2_ != last2_ and first1_ != last1_)
		{
			if (*first1_ < *first2_)
				break;
			else if (*first1_ == *first2_)
			{
				++first1_;
				++first2_;
			}
			else
				++first2_;
		}
	}

	void increment()
	{
		++first1_;
		next_value();
	}
	auto dereference() const -> decltype(*first1_) { return *first1_; }
	bool equal(set_difference_iterator const& r) const { return first1_ == r.first1_; }
};

template<typename R1, typename R2> auto set_difference(R1&& range1, R2&& range2)
{
	using Iter1 = range_begin_type_t<R1>;
	using Iter2 = range_begin_type_t<R2>;
	using Iterator = set_difference_iterator<Iter1, Iter2>;
	auto view1 = all(RAH_STD::forward<R1>(range1));
	auto view2 = all(RAH_STD::forward<R2>(range2));
	return iterator_range<Iterator>{
		{ Iterator(rah_begin(view1), rah_end(view1), rah_begin(view2), rah_end(view2)) },
		{ Iterator(rah_end(view1), rah_end(view1), rah_end(view2), rah_end(view2)) },
	};
}

template<typename R2> auto set_difference(R2&& range2)
{
	return make_pipeable([r2 = range2 | view::all()](auto&& range) {return set_difference(range, r2); });
}

// ********************************** for_each ****************************************************

template<typename R, typename F> auto for_each(R&& range, F&& func)
{
	return range | RAH_NAMESPACE::view::transform(func) | RAH_NAMESPACE::view::join();
}

template<typename F>
inline auto for_each(F&& func)
{
	return make_pipeable([=](auto&& range)
		{
			return RAH_NAMESPACE::view::for_each(RAH_STD::forward<decltype(range)>(range), func);
		});
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
	auto view = all(RAH_STD::forward<R>(range));
	auto b_in = rah_begin(view);
	auto e_in = rah_end(view);
	auto b_out = findIter(b_in, e_in, begin_idx);
	auto e_out = findIter(b_in, e_in, end_idx);
	return iterator_range<decltype(b_out)>{ {b_out}, { e_out } };
}

inline auto slice(intptr_t begin, intptr_t end)
{
	return make_pipeable([=](auto&& range)
		{
			return slice(RAH_STD::forward<decltype(range)>(range), begin, end);
		});
}

// ***************************************** stride ***********************************************

template<typename R>
struct stride_iterator : iterator_facade<stride_iterator<R>, range_ref_type_t<R>, range_iter_categ_t<R>>
{
	range_begin_type_t<R> iter_;
	range_end_type_t<R> end_;
	size_t step_;

	stride_iterator() = default;
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
	bool equal(stride_iterator const& other) const { return iter_ == other.iter_; }
	auto distance_to(stride_iterator const other) const { return (iter_ - other.iter_) / step_; }
};


template<typename R> auto stride(R&& range, size_t step)
{
	auto view = all(RAH_STD::forward<R>(range));
	auto iter = rah_begin(view);
	auto endIter = rah_end(view);
	return iterator_range<stride_iterator<RAH_STD::remove_reference_t<R>>>{
		{ iter, endIter, step}, { endIter, endIter, step }};
}

inline auto stride(size_t step)
{
	return make_pipeable([=](auto&& range)
		{
			return stride(RAH_STD::forward<decltype(range)>(range), step);
		});
}

// ***************************************** retro ************************************************

// Use reverse instead of retro
template<typename R> [[deprecated]] auto retro(R&& range)
{
	auto view = all(RAH_STD::forward<R>(range));
	return make_iterator_range(
		RAH_STD::make_reverse_iterator(rah_end(view)), RAH_STD::make_reverse_iterator(rah_begin(view)));
}

// Use reverse instead of retro
[[deprecated]] inline auto retro()
{
	return make_pipeable([=](auto&& range)
		{
			return retro(RAH_STD::forward<decltype(range)>(range));
		});
}

// ***************************************** reverse **********************************************

template<typename R> auto reverse(R&& range)
{
	auto view = all(RAH_STD::forward<R>(range));
	return make_iterator_range(
		RAH_STD::make_reverse_iterator(rah_end(view)), RAH_STD::make_reverse_iterator(rah_begin(view)));
}

inline auto reverse()
{
	return make_pipeable([=](auto&& range)
		{
			return reverse(RAH_STD::forward<decltype(range)>(range));
		});
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
	auto distance_to(zip_iterator const& other) const { return RAH_STD::get<0>(iters_) - RAH_STD::get<0>(other.iters_); }
	bool equal(zip_iterator const& other) const { return details::equal(iters_, other.iters_); }
};

template<typename ...R> auto zip(R&&... _ranges)
{
	auto views = RAH_STD::make_tuple(all(RAH_STD::forward<R>(_ranges))...);
	auto iterTup = details::transform_each(views, [](auto&& v){ return rah_begin(v);});
	auto endTup = details::transform_each(views, [](auto&& v){ return rah_end(v);});
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

	chunk_iterator() = default;
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
	bool equal(chunk_iterator const& other) const { return iter_ == other.iter_; }
};

template<typename R> auto chunk(R&& range, size_t step)
{
	auto view = all(RAH_STD::forward<R>(range));
	auto iter = rah_begin(view);
	auto endIter = rah_end(view);
	using iterator = chunk_iterator<RAH_STD::remove_reference_t<R>>;
	iterator begin = { iter, iter, endIter, step };
	begin.increment();
	return iterator_range<iterator>{ { begin }, { endIter, endIter, endIter, step }};
}

inline auto chunk(size_t step)
{
	return make_pipeable([=](auto&& range)
		{
			return chunk(RAH_STD::forward<decltype(range)>(range), step);
		});
}

// ***************************************** filter ***********************************************

template<typename R, typename F>
struct filter_iterator : iterator_facade<filter_iterator<R, F>, range_ref_type_t<R>, RAH_STD::bidirectional_iterator_tag>
{
	using Iterator = range_begin_type_t<R>;
	Iterator begin_;
	Iterator iter_;
	range_end_type_t<R> end_;
	RAH_NAMESPACE::details::optional<F> func_;
	typename RAH_STD::iterator_traits<range_begin_type_t<R>>::pointer value_pointer_;

	// Get a pointer to the pointed value,
	//   OR a pointer to a copy of the pointed value (when not a reference iterator)
	template <class I> struct get_pointer
	{
		static auto get(I const& iter) { return iter.operator->(); }
	};
	template <class V> struct get_pointer<V*>
	{
		static auto get(V* ptr) { return ptr; }
	};

	filter_iterator() = default;
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
		while (iter_ != end_ && not (*func_)(*(value_pointer_ = get_pointer<Iterator>::get(iter_))))
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
	bool equal(filter_iterator const& other) const { return iter_ == other.iter_; }
};

template<typename R, typename P> auto filter(R&& range, P&& pred)
{
	auto view = all(RAH_STD::forward<R>(range));
	auto iter = rah_begin(view);
	auto endIter = rah_end(view);
	using Predicate = RAH_STD::remove_cv_t<RAH_STD::remove_reference_t<P>>;
	return iterator_range<filter_iterator<RAH_STD::remove_reference_t<R>, Predicate>>{
		{ iter, iter, endIter, pred },
		{ iter, endIter, endIter, pred }
	};
}

template<typename P> auto filter(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return filter(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ***************************************** concat ***********************************************

template<typename IterPair, typename V>
struct concat_iterator : iterator_facade<concat_iterator<IterPair, V>, V, RAH_STD::forward_iterator_tag>
{
	IterPair iter_;
	IterPair end_;
	size_t range_index_;

	concat_iterator() = default;
	concat_iterator(IterPair const& iter, IterPair const& end, size_t range_index)
		: iter_(iter), end_(end), range_index_(range_index)
	{
		if (range_index == 0)
		{
			if(RAH_STD::get<0>(iter_) == RAH_STD::get<0>(end_))
				range_index_ = 1;
		}
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

	bool equal(concat_iterator const& other) const
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
	auto view1 = all(RAH_STD::forward<R1>(range1));
	auto view2 = all(RAH_STD::forward<R2>(range2));
	auto begin_range1 = RAH_STD::make_pair(rah_begin(view1), rah_begin(view2));
	auto begin_range2 = RAH_STD::make_pair(rah_end(view1), rah_end(view2));
	auto end_range1 = RAH_STD::make_pair(rah_end(view1), rah_end(view2));
	auto end_range2 = RAH_STD::make_pair(rah_end(view1), rah_end(view2));
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
	auto view = all(RAH_STD::forward<R>(range));
	size_t const dist = RAH_STD::distance(rah_begin(view), rah_end(view));
	return zip(iota(size_t(0), dist), view);
}

inline auto enumerate()
{
	return make_pipeable([=](auto&& range)
		{
			return enumerate(RAH_STD::forward<decltype(range)>(range));
		});
}

// ****************************** map_value ********************************************************

template<size_t I>
struct get_tuple_elt
{
	template<typename T>
	auto operator()(T&& nvp) const -> decltype(RAH_STD::get<I>(RAH_STD::forward<decltype(nvp)>(nvp)))
	{
		static_assert(not RAH_STD::is_rvalue_reference<decltype(nvp)>::value,
			"map_value/map_key only apply only apply on lvalue pairs. "
			"Pairs from map are ok but for generated pairs, prefer use view::tranform");
		return RAH_STD::get<I>(RAH_STD::forward<decltype(nvp)>(nvp));
	}
};

template<typename R> auto map_value(R&& range)
{
	return transform(RAH_STD::forward<R>(range), get_tuple_elt<1>{});
}

inline auto map_value()
{
	return make_pipeable([=](auto&& range)
		{
			return map_value(RAH_STD::forward<decltype(range)>(range));
		});
}

// ****************************** map_key **********************************************************

template<typename R> auto map_key(R&& range)
{
	return RAH_NAMESPACE::view::transform(RAH_STD::forward<R>(range), get_tuple_elt<0>{});
}

inline auto map_key()
{
	return make_pipeable([=](auto&& range)
		{
			return map_key(RAH_STD::forward<decltype(range)>(range));
		});
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
	using Container = typename RAH_STD::vector<RAH_STD::remove_cv_t<value_type>>;
	Container result;
	auto view = all(RAH_STD::forward<R>(range));
	result.reserve(RAH_STD::distance(rah_begin(view), rah_end(view)));
	RAH_STD::copy(rah_begin(view), rah_end(view), RAH_STD::back_inserter(result));
	RAH_STD::sort(rah_begin(result), rah_end(result), pred);
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
	return make_pipeable([=](auto&& range)
		{
			return view::sort(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

} // namespace view

// ****************************************** empty ***********************************************

/// @brief Check if the range if empty
///
/// @snippet test.cpp rah::empty
template<typename R> bool empty(R&& range)
{
	return rah_begin(range) == rah_end(range);
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
	auto view = view::all(RAH_STD::forward<R>(range));
	auto pair = RAH_STD::equal_range(rah_begin(view), rah_end(view), RAH_STD::forward<V>(value));
	return make_iterator_range(RAH_STD::get<0>(pair), RAH_STD::get<1>(pair));
}

/// @brief Returns a range containing all elements equivalent to value in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::equal_range_pipeable
template<typename V> auto equal_range(V&& value)
{
	return make_pipeable([=](auto&& range)
		{
			return equal_range(RAH_STD::forward<decltype(range)>(range), value);
		});
}

/// @brief Returns a range containing all elements equivalent to value in the range
///
/// @snippet test.cpp rah::equal_range_pred_0
/// @snippet test.cpp rah::equal_range_pred
template<typename R, typename V, typename P>
auto equal_range(R&& range, V&& value, P&& pred)
{
	auto view = view::all(RAH_STD::forward<R>(range));
	auto pair = RAH_STD::equal_range(rah_begin(view), rah_end(view), RAH_STD::forward<V>(value), RAH_STD::forward<P>(pred));
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
	return make_pipeable([=](auto&& range)
		{
			return equal_range(RAH_STD::forward<decltype(range)>(range), value, pred);
		});
}

// ****************************************** binary_search ***********************************************

/// @brief Checks if an element equivalent to value appears within the range
///
/// @snippet test.cpp rah::binary_search
template<typename R, typename V> auto binary_search(R&& range, V&& value)
{
	return RAH_STD::binary_search(rah_begin(range), rah_end(range), RAH_STD::forward<V>(value));
}

/// @brief Checks if an element equivalent to value appears within the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::binary_search_pipeable
template<typename V> auto binary_search(V&& value)
{
	return make_pipeable([=](auto&& range)
		{
			return binary_search(RAH_STD::forward<decltype(range)>(range), value);
		});
}

// ****************************************** transform *******************************************

/// @brief Applies the given function unary_op to the range rangeIn and stores the result in the range rangeOut
///
/// @snippet test.cpp rah::transform3
template<typename RI, typename RO, typename F>
auto transform(RI&& rangeIn, RO&& rangeOut, F&& unary_op)
{
	return RAH_STD::transform(rah_begin(rangeIn), rah_end(rangeIn), rah_begin(rangeOut), RAH_STD::forward<F>(unary_op));
}

/// @brief The binary operation binary_op is applied to pairs of elements from two ranges
///
/// @snippet test.cpp rah::transform4
template<typename RI1, typename RI2, typename RO, typename F>
auto transform(RI1&& rangeIn1, RI2&& rangeIn2, RO&& rangeOut, F&& binary_op)
{
	return RAH_STD::transform(
		rah_begin(rangeIn1), rah_end(rangeIn1),
		rah_begin(rangeIn2),
		rah_begin(rangeOut),
		RAH_STD::forward<F>(binary_op));
}

// ********************************************* reduce *******************************************

/// @brief Executes a reducer function on each element of the range, resulting in a single output value
///
/// @snippet test.cpp rah::reduce
template<typename R, typename I, typename F> auto reduce(R&& range, I&& init, F&& reducer)
{
	return RAH_STD::accumulate(rah_begin(range), rah_end(range), RAH_STD::forward<I>(init), RAH_STD::forward<F>(reducer));
}

/// @brief Executes a reducer function on each element of the range, resulting in a single output value
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::reduce_pipeable
template<typename I, typename F>
auto reduce(I&& init, F&& reducer)
{
	return make_pipeable([=](auto&& range)
		{
			return reduce(RAH_STD::forward<decltype(range)>(range), init, reducer);
		});
}

// ************************* any_of *******************************************

/// @brief Checks if unary predicate pred returns true for at least one element in the range
///
/// @snippet test.cpp rah::any_of
template<typename R, typename F> bool any_of(R&& range, F&& pred)
{
	return RAH_STD::any_of(rah_begin(range), rah_end(range), RAH_STD::forward<F>(pred));
}

/// @brief Checks if unary predicate pred returns true for at least one element in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::any_of_pipeable
template<typename P> auto any_of(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return any_of(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ************************* all_of *******************************************

/// @brief Checks if unary predicate pred returns true for all elements in the range
///
/// @snippet test.cpp rah::all_of
template<typename R, typename P> bool all_of(R&& range, P&& pred)
{
	return RAH_STD::all_of(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Checks if unary predicate pred returns true for all elements in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::all_of_pipeable
template<typename P> auto all_of(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return all_of(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ************************* none_of *******************************************

/// @brief Checks if unary predicate pred returns true for no elements in the range
///
/// @snippet test.cpp rah::none_of
template<typename R, typename P> bool none_of(R&& range, P&& pred)
{
	return RAH_STD::none_of(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Checks if unary predicate pred returns true for no elements in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::none_of_pipeable
template<typename P> auto none_of(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return none_of(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ************************* count ****************************************************************

/// @brief Counts the elements that are equal to value
///
/// @snippet test.cpp rah::count
template<typename R, typename V> auto count(R&& range, V&& value)
{
	return RAH_STD::count(rah_begin(range), rah_end(range), RAH_STD::forward<V>(value));
}

/// @brief Counts the elements that are equal to value
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::count_pipeable
template<typename V> auto count(V&& value)
{
	return make_pipeable([=](auto&& range)
		{
			return count(RAH_STD::forward<decltype(range)>(range), value);
		});
}

/// @brief Counts elements for which predicate pred returns true
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::count_if
template<typename R, typename P> auto count_if(R&& range, P&& pred)
{
	return RAH_STD::count_if(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Counts elements for which predicate pred returns true
///
/// @snippet test.cpp rah::count_if_pipeable
template<typename P> auto count_if(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return count_if(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ************************* foreach **************************************************************

/// @brief Applies the given function func to each element of the range
///
/// @snippet test.cpp rah::for_each
template<typename R, typename F> auto for_each(R&& range, F&& func)
{
	return ::RAH_STD::for_each(rah_begin(range), rah_end(range), RAH_STD::forward<F>(func));
}

/// @brief Applies the given function func to each element of the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::for_each_pipeable
template<typename F> auto for_each(F&& func)
{
	return make_pipeable([=](auto&& range)
		{
			return for_each(RAH_STD::forward<decltype(range)>(range), func);
		});
}

// ***************************** to_container *****************************************************

/// @brief Return a container of type C, filled with the content of range
///
/// @snippet test.cpp rah::to_container
template<typename C, typename R> auto to_container(R&& range)
{
	return C(rah_begin(range), rah_end(range));
}

/// @brief Return a container of type C, filled with the content of range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::to_container_pipeable
template<typename C> auto to_container()
{
	return make_pipeable([=](auto&& range)
		{
			return to_container<C>(RAH_STD::forward<decltype(range)>(range));
		});
}

// ************************* mismatch *************************************************************

/// @brief Finds the first position where two ranges differ
///
/// @snippet test.cpp rah::mismatch
template<typename R1, typename R2> auto mismatch(R1&& range1, R2&& range2)
{
	return RAH_STD::mismatch(rah_begin(range1), rah_end(range1), rah_begin(range2), rah_end(range2));
}

// ****************************************** find ************************************************

/// @brief Finds the first element equal to value
///
/// @snippet test.cpp rah::find
template<typename R, typename V> auto find(R&& range, V&& value)
{
	return RAH_STD::find(rah_begin(range), rah_end(range), RAH_STD::forward<V>(value));
}

/// @brief Finds the first element equal to value
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::find_pipeable
template<typename V> auto find(V&& value)
{
	return make_pipeable([=](auto&& range)
		{
			return find(RAH_STD::forward<decltype(range)>(range), value);
		});
}

/// @brief Finds the first element satisfying specific criteria
///
/// @snippet test.cpp rah::find_if
template<typename R, typename P> auto find_if(R&& range, P&& pred)
{
	return RAH_STD::find_if(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Finds the first element satisfying specific criteria
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::find_if_pipeable
template<typename P> auto find_if(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return find_if(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

/// @brief Finds the first element not satisfying specific criteria
///
/// @snippet test.cpp rah::find_if_not
template<typename R, typename P> auto find_if_not(R&& range, P&& pred)
{
	return RAH_STD::find_if_not(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Finds the first element not satisfying specific criteria
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::find_if_not_pipeable
template<typename P> auto find_if_not(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return find_if_not(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ************************************* max_element **********************************************

/// @brief Finds the greatest element in the range
///
/// @snippet test.cpp rah::max_element
template<typename R, RAH_STD::enable_if_t<is_range<R>::value, int> = 0>
auto max_element(R&& range)
{
	return RAH_STD::max_element(rah_begin(range), rah_end(range));
}

/// @brief Finds the greatest element in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::max_element_pipeable
inline auto max_element()
{
	return make_pipeable([=](auto&& range)
		{
			return max_element(RAH_STD::forward<decltype(range)>(range));
		});
}

/// @brief Finds the greatest element in the range
///
/// @snippet test.cpp rah::max_element_pred
template<typename R, typename P> auto max_element(R&& range, P&& pred)
{
	return RAH_STD::max_element(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Finds the greatest element in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::max_element_pred_pipeable
template<typename P, RAH_STD::enable_if_t<!is_range<P>::value, int> = 0>
auto max_element(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return max_element(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// ************************************* min_element **********************************************

/// @brief Finds the smallest element in the range
///
/// @snippet test.cpp rah::min_element
template<typename R, RAH_STD::enable_if_t<is_range<R>::value, int> = 0>
auto min_element(R&& range)
{
	return RAH_STD::min_element(rah_begin(range), rah_end(range));
}

/// @brief Finds the smallest element in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::min_element_pipeable
inline auto min_element()
{
	return make_pipeable([=](auto&& range)
		{
			return min_element(RAH_STD::forward<decltype(range)>(range));
		});
}

/// @brief Finds the smallest element in the range
///
/// @snippet test.cpp rah::min_element_pred
template<typename R, typename P> auto min_element(R&& range, P&& pred)
{
	return RAH_STD::min_element(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Finds the smallest element in the range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::min_element_pred_pipeable
template<typename P, RAH_STD::enable_if_t<!is_range<P>::value, int> = 0>
auto min_element(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return min_element(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// *************************************** copy ***************************************************

/// @brief Copy in range into an other
/// @return The part of out after the copied part
///
/// @snippet test.cpp rah::copy
template<typename R1, typename R2> auto copy(R1&& in, R2&& out)
{
	return RAH_STD::copy(rah_begin(in), rah_end(in), rah_begin(out));
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
	return RAH_STD::fill(rah_begin(in), rah_end(in), value);
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
	return RAH_STD::copy_if(rah_begin(in), rah_end(in), rah_begin(out), RAH_STD::forward<P>(pred));
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
	return RAH_STD::distance(rah_begin(range), rah_end(range));
}

/// @brief Get the size of range
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::size_pipeable
inline auto size()
{
	return make_pipeable([=](auto&& range)
		{
			return size(RAH_STD::forward<decltype(range)>(range));
		});
}

// *************************************** equal **************************************************

/// @brief Determines if two sets of elements are the same
///
/// @snippet test.cpp rah::equal
template<typename R1, typename R2> auto equal(R1&& range1, R2&& range2)
{
#ifdef EASTL_VERSION
	return RAH_STD::identical(rah_begin(range1), rah_end(range1), rah_begin(range2), rah_end(range2));
#else
	return RAH_STD::equal(rah_begin(range1), rah_end(range1), rah_begin(range2), rah_end(range2));
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
	S* stream_ = nullptr;
	stream_inserter_iterator() = default;
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
	return RAH_STD::remove_if(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @brief Keep at the begining of the range only elements for which pred(elt) is false\n
/// @return The (end) part of the range to erase.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::remove_if_pipeable
template<typename P> auto remove_if(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return remove_if(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// *********************************** remove *****************************************************

/// @brief Keep at the begining of the range only elements not equal to value\n
/// @return Return iterator to the part of the range to erase.
///
/// @snippet test.cpp rah::remove
template<typename R, typename V> auto remove(R&& range, V&& value)
{
	return RAH_STD::remove(rah_begin(range), rah_end(range), RAH_STD::forward<V>(value));
}

/// @brief Keep at the begining of the range only elements not equal to value\n
/// @return Return iterator to the part of the range to erase.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::remove_pipeable
template<typename V> auto remove(V&& value)
{
	return make_pipeable([=](auto&& range)
		{
			return remove(RAH_STD::forward<decltype(range)>(range), value);
		});
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
	return RAH_STD::partition(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @see rah::partition(R&&, P&&)
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::partition_pipeable
template<typename P> auto partition(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return partition(RAH_STD::forward<decltype(range)>(range), pred);
		});
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
	return RAH_STD::stable_partition(rah_begin(range), rah_end(range), RAH_STD::forward<P>(pred));
}

/// @see rah::stable_partition(R&&, P&&)
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::stable_partition_pipeable
template<typename P> auto stable_partition(P&& pred)
{
	return make_pipeable([=](auto&& range)
		{
			return stable_partition(RAH_STD::forward<decltype(range)>(range), pred);
		});
}

// *********************************** erase ******************************************************

/// @brief Erase a sub-range of a given container
/// @return A view on the resulting container
///
/// @snippet test.cpp rah::erase
template<typename C, typename R> auto erase(C&& container, R&& subrange)
{
	container.erase(rah_begin(subrange), rah_end(subrange));
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
	RAH_STD::sort(rah_begin(range), rah_end(range), pred);
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
	RAH_STD::stable_sort(rah_begin(range), rah_end(range), pred);
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
	RAH_STD::shuffle(rah_begin(range), rah_end(range), RAH_STD::forward<URBG>(g));
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
	return RAH_STD::unique(rah_begin(range), rah_end(range), pred);
}

/// @brief Remove all but first successive values which are equals. Without resizing the range.
/// @return The end part of the range, which have to be remove.
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::unique_pipeable
/// @snippet test.cpp rah::unique_pred_pipeable
template<typename P = is_equal, typename = RAH_STD::enable_if_t<not is_range<P>::value>>
auto unique(P&& pred = {})
{
	return make_pipeable([=](auto&& range)
		{
			return unique(RAH_STD::forward<decltype(range)>(range), pred);
		});
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
		rah_begin(in1), rah_end(in1),
		rah_begin(in2), rah_end(in2),
		rah_begin(out));
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
		rah_begin(in1), rah_end(in1),
		rah_begin(in2), rah_end(in2),
		rah_begin(out));
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
	return make_pipeable([=](auto&& range) -> auto&&
	{
		return action::unique(RAH_STD::forward<decltype(range)>(range), pred);
	});
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
	return make_pipeable([=](auto&& range) -> auto&&
	{
		return remove_if(RAH_STD::forward<decltype(range)>(range), pred);
	});
}

// *********************************** remove *****************************************************

/// @brief Keep only elements not equal to @b value
/// @return reference to the container
///
/// @snippet test.cpp rah::action::remove
template<typename C, typename V> auto&& remove(C&& container, V&& value)
{
	container.erase(RAH_NAMESPACE::remove(container, RAH_STD::forward<V>(value)), container.end());
	return RAH_STD::forward<C>(container);
}

/// @brief Keep only elements not equal to @b value
/// @return reference to the container
/// @remark pipeable syntax
///
/// @snippet test.cpp rah::action::remove_pipeable
template<typename V> auto remove(V&& value)
{
	return make_pipeable([=](auto&& range) -> auto&&
	{
		return remove(RAH_STD::forward<decltype(range)>(range), value);
	});
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
	return make_pipeable([=](auto&& range) -> auto&&
	{
		return action::sort(RAH_STD::forward<decltype(range)>(range), pred);
	});
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
	RAH_STD::fill(rah_begin(in), rah_end(in), value);
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
