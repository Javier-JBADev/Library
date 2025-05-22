/*
 * MIT License
 * Copyright (c) 2025 Javier Benito Abolafio
 * contact: javier@jba.dev
 * github: https://github.com/Javier-JBADev/Projects
 */

#pragma once
#include <vector>
#include <functional>

// -------------------------------
// Declares a multicast delegate with **no parameters**
//
// Usage:
//   DECLARE_MULTICAST_DELEGATE(MyDelegate);
//   MyDelegate OnSomethingHappened;
//
//   OnSomethingHappened.Add(obj, &MyClass::Method);
//   OnSomethingHappened.Broadcast(); // No args
//
// This will cause a **compile-time error** if you accidentally provide parameters.
// -------------------------------
#define DECLARE_MULTICAST_DELEGATE(DelegateName, ...)           \
  ZeroParamCheck<__VA_ARGS__>();                                \
  using DelegateName = MulticastDelegate<>;

// -------------------------------
// Declares a multicast delegate with **one parameter**
//
// Usage:
//   DECLARE_MULTICAST_DELEGATE_OneParam(MyDelegate, int);
//   MyDelegate OnHealthChanged;
//
//   OnHealthChanged.Add(obj, &MyClass::OnHealthChanged);
//   OnHealthChanged.Broadcast(100); // One argument
// -------------------------------
#define DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, ...) \
  using DelegateName = MulticastDelegate<typename OneParam<__VA_ARGS__>::type>;

// -------------------------------
// Declares a multicast delegate with **two parameters**
//
// Usage:
//   DECLARE_MULTICAST_DELEGATE_TwoParam(MyDelegate, int, float);
//   MyDelegate OnPositionChanged;
//
//   OnPositionChanged.Add(obj, &MyClass::OnPositionChanged);
//   OnPositionChanged.Broadcast(10, 3.14f); // Two arguments
// -------------------------------
#define DECLARE_MULTICAST_DELEGATE_TwoParam(DelegateName, ...) \
  using DelegateName = MulticastDelegate<typename TwoParam<__VA_ARGS__>::type>;

template<typename... Args>
class IDelegateBase
{
  public:
    virtual ~IDelegateBase() = default;
    virtual void Execute(Args... args) = 0;
    virtual bool IsBound(void* objectPtr, void* methodPtrRaw) const = 0;
    virtual bool IsExpired() const = 0;
};

template<typename ClassType, typename... Args>
class DelegateInstance : public IDelegateBase<Args...>
{
  public:
    using MethodPtr = void(ClassType::*)(Args...);

    DelegateInstance(std::weak_ptr<ClassType> obj, MethodPtr m) : object(obj), method(m){}

    void Execute(Args... args) override
    {
      if (auto locked = object.lock())
        (locked.get()->*method)(args...);
    }

    bool IsBound(void* objectPtr, void* methodPtrRaw) const override
    {
      auto locked = object.lock();
      MethodPtr incomingMethod = *static_cast<MethodPtr*>(methodPtrRaw);
      return (locked.get() == objectPtr && incomingMethod == method);
    }

    bool IsExpired() const override
    {
      return object.expired();
    }

  private:
    std::weak_ptr<ClassType> object;
    MethodPtr method;
};

template<typename... Args>
class MulticastDelegate
{
  public:
    template<typename ClassType>
    void Add(std::shared_ptr<ClassType> object, void(ClassType::* method)(Args...))
    {
      // Prevent duplicate bindings
      if(!IsBound(object, method))
        vDelegates.push_back(std::make_shared<DelegateInstance<ClassType, Args...>>(object, method));
    }

    template<typename ClassType>
    void Remove(std::shared_ptr<ClassType> object, void (ClassType::* method)(Args...))
    {
      using MethodPtr = void (ClassType::*)(Args...);

      void* methodPtr = static_cast<void*>(&method);

      auto it = std::remove_if(vDelegates.begin(), vDelegates.end(),
                [&](const std::shared_ptr<IDelegateBase<Args...>>& del)
                {
                  // Cast member function pointer to void* pointer and pass the address
                  return del->IsBound(object.get(), methodPtr);
                });

      vDelegates.erase(it, vDelegates.end());
    }

    void RemoveAll()
    {
      vDelegates.clear();
    }

    template<typename ClassType>
    bool IsBound(std::shared_ptr<ClassType> object, void (ClassType::* m)(Args...))
    {
      void* objectPtr = object.get();
      void* methodPtr = static_cast<void*>(&m);

      for(auto& delegate : vDelegates)
      {
        if(delegate->IsBound(objectPtr, methodPtr))
          return true;
      }
      return false;
    }

    void Broadcast(Args... args)
    {
      for(const std::shared_ptr<IDelegateBase<Args...>>& delegate : vDelegates)
      {
        if(!delegate->IsExpired())
          delegate->Execute(args...);
      }
    }

  private:
    std::vector<std::shared_ptr<IDelegateBase<Args...>>> vDelegates;
};

/*
 * Helper structs to enforce that delegate macros receive the correct number of parameters.
 * These are used internally by the DECLARE_MULTICAST_DELEGATE macros to trigger
 * compile-time errors when misused.
 */

// -------------------------------
// Enforces that no parameters are passed.
//
// Used by: DECLARE_MULTICAST_DELEGATE(DelegateName)
//
// Example (valid):
//   DECLARE_MULTICAST_DELEGATE(MyDelegate); // OK
//
// Example (invalid):
//   DECLARE_MULTICAST_DELEGATE(MyDelegate, int); // Triggers static_assert
// -------------------------------
template<typename... Args>
struct ZeroParamCheck {
  static_assert(sizeof...(Args) == 0, "DECLARE_MULTICAST_DELEGATE must not take any parameters.");
};

// -------------------------------
// Extracts the single parameter type and enforces exactly one parameter.
//
// Used by: DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, ParamType)
//
// Example:
//   DECLARE_MULTICAST_DELEGATE_OneParam(MyDelegate, float);
//   // 'type' will resolve to 'float'
//
// Example (invalid):
//   DECLARE_MULTICAST_DELEGATE_OneParam(MyDelegate);             // Error
//   DECLARE_MULTICAST_DELEGATE_OneParam(MyDelegate, int, float); // Error
// -------------------------------
template<typename... Args>
struct OneParam
{
  static_assert(sizeof...(Args) == 1,
    "DECLARE_MULTICAST_DELEGATE_OneParam requires exactly one parameter");
  using type = std::tuple_element_t<0, std::tuple<Args...>>;
};

// -------------------------------
// Extracts the first parameter type and enforces exactly two parameters.
//
// Used by: DECLARE_MULTICAST_DELEGATE_TwoParam(DelegateName, Param1, Param2)
//
// NOTE: Currently only the first parameter type is extracted with `type`,
// since MulticastDelegate<T> expects a single type. You may want to extend this
// if supporting multi-parameter delegates in full.
//
// Example:
//   DECLARE_MULTICAST_DELEGATE_TwoParam(MyDelegate, int, float);
//   // 'type' will resolve to 'int' (adapt MulticastDelegate if needed)
//
// Example (invalid):
//   DECLARE_MULTICAST_DELEGATE_TwoParam(MyDelegate, int); // Error
// -------------------------------
template<typename... Args>
struct TwoParam
{
  static_assert(sizeof...(Args) == 2,
    "DECLARE_MULTICAST_DELEGATE_TwoParam requires exactly one parameter");
  using type = std::tuple_element_t<0, std::tuple<Args...>>;
};