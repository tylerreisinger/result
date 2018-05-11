# result
A C++17 implementation of the "value or error" type Result<T, E>.

## Overview


`Result` is an adaptation of the [std::result::Result](https://doc.rust-lang.org/std/result/enum.Result.html) type from
the Rust programming language to C++.
It is a class for communicating errors in a safe and visible manner. It is a sum type containing either a value
of type T or an error of type E and can be chained together or transformed easily.

Unlike exceptions, which are by
design very easy to ignore and which introduce complexity in handling them safely, a `Result` must be
considered at each step of its propagation. While this leads to some extra verbosity, it also makes it much
easier to reason about the error paths within the code and to reason about what operations may or may not fail.

Also unlike using simple return values, `Result` does not require giving up the return slot of the function to
return errors, or to use special sentinal values. A `Result` fully contains either the normally returned value
or, if an error occured, an arbitrary object describing the error transparently.

`Result` is an adaptation of the [std::result::Result](https://doc.rust-lang.org/std/result/enum.Result.html) type from
the Rust programming language to C++.

**result** does not have any dependencies other than a C++17 compliant compiler.

## Example

```cpp
#include "result/result.h"
#include <cmath>
#include <iostream>

class MyError {
public:
 
  enum class ErrorKind {
      OutOfDomain
  };
  MyError(ErrorKind kind, std::string reason = ""):
    kind(kind), reason(reason) {}
  
  ErrorKind kind;
  std::string reason;
}



result::Result<double, MyError> checked_log(double x) {
  if(x <= 0.0) {
    return result::Err(MyError(MyError::ErrorKind::OutOfDomain, "out of domain"));
  } else {
    return result::Ok(std::log(x));
  }
}

int main() {
  double value;
  std::cin >> value;
  auto result = checked_log(value);
  std::cout << "Your log is: " << result.expect("Given value is outside of the log domain") << std::endl
  return 0;
}
```

## Details

**result** is designed to be very fast and flexible.

* **Fast** -- `Result` favors move semantics heavily and tries to prevent copying in/out of a `Result` 
 as much as possible. It takes callback functions as template arguments for easy inlining.
* **Flexible** -- `Result` is mostly `constexpr` friendly with helper functions that can be utilized with
 many different styles.

### Construction, and the wrapper types `Ok` and `Err`

`Ok<T>` and `Err<T>` are light wrappers over a value of type `T`. They are the most common way to construct
Result objects, either directly as in `Result<int, std::string>(Ok(5));` or by implicit casting to a result type,
such as through a function return. When talking about a Result, it is said to contain an `Ok` object or an `Err` object.

`Result` can also be constructed in-place with the helper tags `ok_tag_t` and `err_tag_t`. Simply pass the tag as the
first argument to the constructor, and the remainer of the arguments are forwarded to the inner object's constructor.

```cpp
extern std::vector<int> other_vec;
Result<std::vector<int>, std::string>(ok_tag_t, other_vec.begin(), other_vec.end())
```

### Querying state

If you want know whether an operation has succeeded or failed, the functions `Result::is_ok()` and  `Result::is_err()`
return whether the `Result` is an `Ok` or `Err` respectively. Additionally, `Result` can be implicitly cast to a bool
which evaluates to true if the `Result` is `Ok`.

To obtain the value inside the `Result`, there are a few methods:

* `Result::unwrap()` and `Result::unwrap_err()`
  
  These methods return, by *moved value*, the `Ok` or `Err` object, respectively. If the `Result` does not contain
  the requested type, then the function *errors*, which by default calls `std::terminate()` and prints out a message.
  Thus, these functions should be used only when it is known what the `Result` contains, or when the error is unable
  to be handled. Note that since this moves out of the `Result`, the `Result` should not be used afterward.
  
  ```cpp
  Result<int, std::string> result = Ok(5);
  Result<int, std::string> result2 = Err("bad operation"s);
  assert(result.unwrap() == 5);
  //result.unwrap_err() errors
  assert(result2.unwrap_err() == "bad operation"s);
  //result2.unwrap() errors
  ```
  
* `Result::expect(str)` and `Result::expect_err(str)`

  Very similar to `unwrap` and `unwrap_err`, `expect*` returns the `Ok` or `Err` object, or errors if the `Result` does
  not contain what you asked. Unlike `unwrap`, `expect` allows you to provide a custom error message to be passed to the
  error function.
  
* `Result::ok()` and `Result::err()`

  `ok()` and `err()` convert a `Result<T, E>` into an `std::optional<T>`. If `Result` holds an `Ok`,
  the `std::optional<T>` will contain a value whereas for `Err` the `optional` will hold `nullopt`. 
  This effectively strips the error or value information from the `Result`. For an l-value `Result`, the returned
  optional value holds a `std::reference_wrapper<T>` to prevent copying, whereas an r-value `Result` will move out
  the value.
  
* `Result::try_ok()` and `Result::try_err()`

  Similar to `unwrap` and `unwrap_err`, except returns a reference to the contained data instead of moving it out
  of the `Result`. Like `unwrap`, it will error if the `Result` does not contain the requested value.
  
* `Result::unwrap_or(x)` and `Result::unwrap_err_or(x)`

  Behaves the same as `unwrap` if the `Result` is an `Ok`, or returns the provided default value if it is an `Err`.
  
  ```cpp
  Result<int, std::string> result(Err("boom"s));
  Result<int, std::string> result2(Ok(500));
  assert(result.unwrap_or(10) == 10);
  assert(result.unwrap_err_or("meow"s) == "boom"s);
  assert(ressult2.unwrap_or(10) == 500);
  assert(ressult2.unwrap_err_or("meow"s) == "meow"s);
  ```

* `Result::unwrap_or_default()` and `Result::unwrap_err_or_default()`

  Behaves the same as `unwrap` and `unwrap_err` if the `Result` contains the requested type, 
  or returns the default value if it does.
  Equivalent to `Result<T, E>::unwrap_or(T())` and `Result<T, E>::unwrap_err_or(E())`.
  
  Only defined if `T` or `E` have a default constructor, respectively.
  
### Chaining and modifiers

  `Result` provides some helper methods for combining or modifying a `Result` without inspecting its state directly.
  All of these methods move the values from the original `Result` and return a new `Result`.
  
  Any method taking a function type (denoted as `fn`) can take any C++ callable object that is compatible with the
  signature, be it a plain old function, a lambda, a `std::function` or anything with `operator ()`.
  
* `Result<T, E>::map(fn) -> Result<U, E>` and `Result<T, E>::map_err(fn2) -> Result<T, F>`

  Function signature: `auto fn(T) -> U` and `auto fn(E) -> F`


  `map` and `map_err` modify the value and/or type of the `Ok` or `Err` in a `Result` respectively. The
  function is only called if the `Result` kind matches the function's target. If it does not, the object
  is moved unmodified to the returned result.
  
  The taken function receives the value of `Ok` and transforms it to a new value.
  
  ```cpp
  Result<int, std::string> result = Ok(4);
  //Copy so we can reuse the initial result
  assert(result.clone().map([](auto x){ return static_cast<double>(x) * 2.0; }) == Ok(8.0));
  assert(result.map_err([](auto){ return 5; }) == Ok(4));
  assert(Result<int, std::string>(Err("dog"s)).map_err([](auto){ return "cat"s; }) == "cat"s);
  ```
  
* `Result<T, E>::and_(Result<U, E> res) -> Result<U, E>`

  If the first `Result` is `Ok`, then return the second, otherwise return the first result's `Err`.
  
  Effectively, this returns `Ok` only if both `Results` are `Ok`, otherwise it returns the first `Err` encountered.
  
  Note: The trailing underscore prevents conflict with the alternative operator name `and`.
  
  ```cpp
  Result<int, std::string> result1 = Ok(5);
  Result<int, std::string> result2 = Ok(10);
  Result<int, std::string> result3 = Err("bad"s);
  
  assert(result1.clone().and_(result2.clone()) == Ok(10));
  assert(result1.and_(result3) == Err("bad"s));
  ```
  
* `Result<T, E>::and_then(fn) -> Result<U, E>`

  Function signature: `auto fn(T) -> Result<U, E>`.

  Similar to `and_`, but instead of taking a second result as an argument, it takes a function that returns the result.
  
  This is useful in that it allows short circuiting. If the first result is `Err`, then the function will never be
  evaluated, otherwise the function will be evaluated once and the result returned.
  
* `Result<T, E>::or_(Result<T, F> res) -> Result<T, F>`

  An opposite analog to `and_`. If the first result is `Ok` return it, otherwise return the second result.
  
  Effectively, this returns the first `Ok` value encountered. Any errors that are present in the first result are 
  ignored.
  
  ```cpp
  Result<int, std::string> result1 = Ok(5);
  Result<int, std::string> result2 = Ok(10);
  Result<int, std::string> result3 = Err("bad"s);
  
  assert(result1.clone().or_(result2.clone()) == Ok(5));
  assert(result3.or_(result1) == Ok(5));
  ```
  
* `Result<T, E>::or_else(fn) -> Result<T, F>`

  Function signature: `auto fn(E) -> Result<T, F>`.
  
  Similar to `or_`, but instead of taking a second result as an argument, it takes a function that returns the result.
  
  This allows short circuiting like `and_then` does. If the first result is `Ok` then the function will never be
  evaluated, otherwise it will be evaluated once and the result returned.
  
### `unit_t`

  Use of `void` in templates can cause issues as it does not behave like a normal type and requires a lot of
  `enable_if`s and template specializations to make work, yet a `Result<void, E>` type is useful in many situations.
  To facilitate this idea, `unit_t` is a unit struct defined by **result** that is trivially constructable, holds no
  data, and is always equal to any other `unit_t`. A `Result<unit_t, E>` behaves just like the void one should,
  but without the headache. To construct such a `Result`, you can use `Result<unit_t, E>(Ok());`.
  
  Note that `Result` cannot have a `unit_t` error type. If you want a "null" error, use `std::optional<T>` instead.

### Other Niceties

 The `Result` type has a few other utility features that ease its use: 
 
 * It overloads all the standard comparison operators, allowing Result objects to be compared like their containing
   value, while placing all errors at the end.
 * It overloads std::hash for hashable types, allowing it to be placed into a hash table.
 * Overloads for operator << are provided on most of the defined types.
 
 
### Performance Considerations

 **result** was designed to maximize reliance on move semantics and minimize all unnecessary copying. `clone` is 
 the only member function that should copy the `Result` or any of its components. Additionally, all sensible functions
 are marked `constexpr` and `Result` should be usable within other `constexpr` functions.
 
 Memory-wise, the memory required by `Result<T, E>` is equal to `max(sizeof(T), sizeof(E))+1`, however it will be
 aligned based on the highest alignment requirement. While a `Result<char, char>` should have a size of 2,
 `Result<int, int>` might have an aligned size of 8.
