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

};

template<typename ClassType, typename... Args>
class DelegateInstance : public IDelegateBase<Args...>
{

  public:
    using MethodPtr = void(ClassType::*)(Args...);

    DelegateInstance(ClassType* pObject, MethodPtr pMethod) : Object(pObject), Method(pMethod) {}

    void Execute(Args... pArgs) override
    {
      
      if(Object && Method)
        (Object->*Method)(pArgs...);

    }

    bool IsBoundTo(ClassType* pObject, MethodPtr pMethod) const
    {
    
      return Object == pObject && Method == pMethod;

    }

  private:
    ClassType* Object;
    MethodPtr Method;

};

template<typename... Args>
class MulticastDelegate
{

  public:
    template<typename ClassType>
    void Add(ClassType* object, void(ClassType::*method)(Args...))
    {      
      vDelegates.push_back(std::make_shared<DelegateInstance<ClassType, Args...>>(object, method));
    }

    void Broadcast(Args... args)
    {
      for(const auto& delegate : vDelegates)
      {
        delegate->Execute(args...);
      }
    }

  private:
    std::vector<std::shared_ptr<IDelegateBase<Args...>>> vDelegates;

};
