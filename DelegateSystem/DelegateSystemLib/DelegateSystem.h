/*
 * MIT License
 * Copyright (c) 2025 Javier Benito Abolafio
 * contact: javier@jba.dev
 * github: https://github.com/Javier-JBADev/Projects
 */

#pragma once
#include <vector>
#include <functional>

template<typename... Args>
class IDelegateBase
{
  public:
    virtual ~IDelegateBase() = default;
    virtual void Execute(Args... args) = 0;
    virtual bool IsBound(void* objectPtr, void* methodPtrRaw) const = 0;
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
        delegate->Execute(args...);
      }
    }

  private:
    std::vector<std::shared_ptr<IDelegateBase<Args...>>> vDelegates;
};