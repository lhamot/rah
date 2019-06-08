
# rah
**rah** is **ra**nge, **h**eader-only, "single-file" C++14/17 library.
## What is a range library?
A range library is an extension of the iterators and STL which allow to use and extend algorithms easily.
A range is anything that can be iterate. Typically in C++ something is a range if we can call `begin(range)` and `end(range)` on it.
## Why a new range library?
Yes there are some great range libraries in C++, like [range-v3](https://github.com/ericniebler/range-v3) and [boost::range](http://www.boost.org/doc/libs/1_70_0/libs/range).
The goal of **rah** is to be "single file" and stay easy to read and understand. **rah** was design with the code simplicity on top priority.
## What is inside rah
- In the namespace `rah`, there is a part of the usual algorithms present in `<algorithm>`, but in a range fashion
	- More algorithms will be added soon
```cpp
// Usual code with STL
std::cout << std::count(range.begin(), range.end(), 2);
// Using range algorithm
std::cout << rah::count(range, 2);
```
- In `rah::view` are functions to create ranges, working only when iterate, saving some memory and simplifying the code, especially when wanting to chain algorithms.
```cpp
// Usual code with STL
std::vector<int> ints;
ints.resize(10);
std::iota(range.begin(), range.end(), 0);
std::vector<int> even;
std::copy_if(ints.begin(), ints.end(), std::back_inserter(even), [](int a) {return a % 2 == 0;});
std::vector<int> values;
std::transform(even.begin(), even.end(), std::back_inserter(values), [](int a) {return a * 2;});
for(int i: values)
    std::cout << i << std::endl;
// Usinging range algorithms
auto values = 
    rah::view::transform(
        rah::view::filter(
            rah::view::iota(0, 10), 
            [](int a) {return a % 2 == 0;})
        [](int a) {return a * 2;});
for(int i: values) // The job in done here, without memory allocation
    std::cout << i << std::endl;
```
- Most of views have a *pipeable* version, making theme easier to write, and read.
```cpp
// Using pipeable range algorithms
auto values = rah::view::iota(0, 10)
    | rah::view::filter([](int a) {return a % 2 == 0;}) 
    | rah::view::transform([](int a) {return a * 2;});
for(int i: values) // The job in done here, without memory allocation
    std::cout << i << std::endl;
``` 
## License
rah is under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt)
## Documentation
You can find the doc [here](https://lhamot.github.io/rah/html/index.html)
## Supported Compilers
- On windows
  - Visual Studio 2017 (stdcpp14 and stdcpp17)
  - clang 7 (-std=c++14 and -std=c++17)
- On Ubuntu
  - clang 4 and 5 (-std=c++14 and -std=c++17)
  - gcc 7,8 and 9 (-std=c++14 and -std=c++17)
## Continuous integration
### Linux
[![Build Status](https://travis-ci.org/lhamot/rah.svg?branch=master)](https://travis-ci.org/lhamot/rah)
### Windows
[![Build status](https://ci.appveyor.com/api/projects/status/kn9yeci2isl6njla/branch/master?svg=true)](https://ci.appveyor.com/project/lhamot/rah/branch/master)
## How to use?
- Just include the `rah.hpp` file in your project
- range version of STL algorithms are in the **rah** namespace
- Views are in **rah::view** namespace

