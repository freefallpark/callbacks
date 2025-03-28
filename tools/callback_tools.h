// Copyright (c) 2025, Ultradent Products Inc. All rights reserved.

#ifndef CALLBACKS_TOOLS_CALLBACK_TOOLS_H_
#define CALLBACKS_TOOLS_CALLBACK_TOOLS_H_

#include <functional>
#include <mutex>

template<class FunctionSignature>
class Callback;

/**
 * @class Callback<Return>(Parameters...)>
 *
 * @brief A thread-safe wrapper around a single std::fucntion<Return(Parameters...)> that can be invoked using the
 * operator().
 *
 * @details This class allows users to store a callback (i.e. any callable matching the specified function signature)
 * and safely call it from multiple threads. The callback can be reassigned at runtime with Register()
 *
 * @forwarding
 * Internally, operator() uses `std::forward<Parameters>(params)...` but **this is not “perfect forwarding.”**
 * Because `Parameters...` are fixed by the template signature, the value category (lvalue/rvalue) is not deduced
 * at the call site in the same way a universal reference (e.g., `T&&`) would be.
 * @forwarding
 * - **If you specify a reference type** (e.g., `int&`) in `Parameters...`, calls will indeed pass a reference,
 *   and modifications to that parameter inside the callback will affect the original object.
 * @forwarding
 * - **If you specify a value type** (e.g., `int`), an rvalue or lvalue argument will always be forwarded
 *   as `int`, potentially leading to a copy (even if you pass an rvalue).
 * @forwarding
 * - This design is perfectly fine for typical usage, but you should not expect “perfect forwarding”
 *   semantics (where rvalues stay rvalues, lvalues stay lvalues).
 *
 * @code
 *  // Create a callback that returns bool and takes an int as a parameter
 *  Callback<bool(int)> callback;
 *  callback.Register([](double d)->bool{ return d > 69;});
 *
 *  bool result = callback(420); // result is true;
 *
 * @endcode
 *
 */
template<class Return, class... Parameters>
class Callback<Return(Parameters...)>{
 public:
  /**
   * @brief FunctionType represents the function signature
   */
  using FunctionType = std::function<Return(Parameters...)>;

  /**
   * @brief Default Constructable
   */
  Callback() noexcept = default;

  // Delete Copy CTOR and Copy Assignment Operator which ensures ownership remains unique
  Callback(Callback&) = delete;
  Callback& operator=(Callback&) = delete;

  // Default Move semantics to allow moving
  Callback(Callback&&) noexcept = default;
  Callback& operator=(Callback&&) noexcept = default;

  /**
   * @brief Trivially Constructable
   */
  ~Callback() noexcept = default;

  /**
   * @brief Assign a method to the callback with this method
   * @param callback Function You would like to store as the callback
   */
  void RegisterCallback(FunctionType callback){
    std::lock_guard lock(mtx_);
    callback_ = std::move(callback);
  }

  /**
   * @overload operator() makes this object callable like a function. It allows users to call the stored callback
   * using the function call syntax. 'x = func()'
   * @param params Parameters of the callback if any
   * @return Returns the Return object defined in the signature (when creating the callback)
   */
  Return operator ()(Parameters... params) const {
    // Protect from Data Races and get a thread-safe, local copy of the callback. This allows us to use this snapshot
    // atomically. Since we don't know how 'heavy' the callback is, we don't necessarily want to block while its
    // being executed. So lets quickly get a copy of that callback and use that copy for the rest of the method.
    FunctionType local_copy;
    {
      std::lock_guard lock(mtx_);
      local_copy = callback_;
    }

    // Use a compile-time check to determine if Return is void
    if constexpr (std::is_void_v<Return>){
      if(!local_copy) return; // Exit early if callback is not valid
      local_copy(std::forward<Parameters>(params)...);
      return;
    }
    // Callback is non-void, we must return something...
    else{
      // Do a compile time check to ensure 'Return' object is default-constructable
      static_assert(std::is_default_constructible_v<Return>);
      if(!local_copy){
        // Return a default constructed Return object if the callback is not valid. This Requires
        // Return to be default-constructable.
        return Return{};
      }
      // Call the valid stored method and return its result;
      return local_copy(std::forward<Parameters>(params)...);
    }
  }

  /**
   * @overload operator bool() Lets you check if(callback) is valid. Marked explicit to prevent implicit conversions to
   * bool (i.e. being used in a function that takes a bool as an input.)
   *
   */
  explicit operator bool() const{
    std::lock_guard lock(mtx_);
    return static_cast<bool>(callback_);
  }

 private:
  mutable std::mutex mtx_;
  FunctionType callback_ = nullptr;
};

#endif //CALLBACKS_TOOLS_CALLBACK_TOOLS_H_
