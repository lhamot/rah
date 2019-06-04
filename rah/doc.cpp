/// @fn rah::make_pipeable(MakeRange&& make_range)
/// @brief Call to create a "pipeable" function (UFCS style in c++)
///
/// @snippet rah.cpp make_pipeable create
/// @snippet rah.cpp make_pipeable use

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
/// @snippet rah.cpp iota

/// @fn rah::view::generate(F&& func)
/// @snippet rah.cpp generate
/// @brief Create an infinite range, repetitively calling func

/// @fn rah::view::generate_n(F&& func, size_t count)
/// @snippet rah.cpp generate_n
/// @brief Create a range of N elemnts, repetitively calling func

/// @fn rah::view::all(R&& range)
/// @brief Create a view on the whole range

/// @fn rah::view::all()
/// @brief Create a view on the whole range
/// @remark pipeable syntax

/// @fn rah::view::transform(R&& range, F&& func)
/// @snippet rah.cpp rah::view::transform
/// @brief Create a view applying a transformation to each element of the input range

/// @fn rah::view::transform(F&& func)
/// @brief Create a view applying a transformation to each element of the input range
/// @remark pipeable syntax
/// @snippet rah.cpp rah::view::transform_pipeable

/// @fn rah::view::slice(R&& range, size_t begin, size_t end)
/// @snippet rah.cpp slice
/// @brief Create a view that is a sub-range of a range

/// @fn rah::view::slice(size_t begin, size_t end)
/// @brief Create a view that is a sub-range of a range
/// @remark pipeable syntax
/// @snippet rah.cpp slice_pipeable

/// @fn rah::view::stride(R&& range, size_t step)
/// @snippet rah.cpp stride
/// @brief Create a view consisting of every Nth element, starting with the first

/// @fn rah::view::stride(size_t step)
/// @brief Create a view consisting of every Nth element, starting with the first
/// @remark pipeable syntax
/// @snippet rah.cpp stride_pipeable

/// @fn rah::view::retro(R&& range)
/// @snippet rah.cpp retro
/// @brief Create a view that traverses the source range in reverse order

/// @fn rah::view::retro()
/// @brief Create a view that traverses the source range in reverse order
/// @remark pipeable syntax
/// @snippet rah.cpp retro_pipeable

/// @fn rah::view::zip(R&&... _ranges)
/// @snippet rah.cpp zip
/// @brief Given N ranges, return a new range where Mth element is the result of calling std::make_tuple on the Mth elements of all N ranges. 

/// @fn rah::view::chunk(R&& range, size_t step)
/// @snippet rah.cpp chunk
/// @brief Create a view where each element is a range of N elements of the input range

/// @fn rah::view::chunk(size_t step)
/// @snippet rah.cpp chunk_pipeable
/// @brief Create a view where each element is a range of N elements of the input range
/// @remark pipeable syntax

/// @fn rah::view::filter(R&& range, F&& func)
/// @snippet rah.cpp filter
/// @brief Create a view with only elements which are filtered

/// @fn rah::view::filter(F&& func)
/// @snippet rah.cpp filter_pipeable
/// @brief Create a view with only elements which are filtered
/// @remark pipeable syntax

/// @fn rah::view::join(R1&& range1, R2&& range2)
/// @snippet rah.cpp join
/// @brief Create a view that is the concatenation of 2 ranges

/// @fn rah::view::join(R&& rightRange)
/// @snippet rah.cpp join_pipeable
/// @brief Create a view that is the concatenation of 2 ranges
/// @remark pipeable syntax

/// @fn rah::view::enumerate(R&& range)
/// @snippet rah.cpp enumerate
/// @brief Pair each element of a range with its index. 

/// @fn rah::view::enumerate()
/// @snippet rah.cpp enumerate_pipeable
/// @brief Pair each element of a range with its index. 
/// @remark pipeable syntax

/// @fn rah::view::map_value(R&& range)
/// @snippet rah.cpp map_value
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the first element of the pair. 

/// @fn rah::view::map_value()
/// @snippet rah.cpp map_value_pipeable
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the first element of the pair. 
/// @remark pipeable syntax

/// @fn rah::view::map_key(R&& range)
/// @snippet rah.cpp map_key
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the second element of the pair. 

/// @fn rah::view::map_key()
/// @snippet rah.cpp map_key_pipeable
/// @brief Given a range of std::pair-std::tuple, create a view consisting of just the second element of the pair. 
/// @remark pipeable syntax
