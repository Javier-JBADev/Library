/*
 * MIT License
 * Copyright (c) 2025 Javier Benito Abolafio
 * contact: javier@jba.dev
 * github: https://github.com/Javier-JBADev/Projects
 */

#pragma once
#include <vector>
#include <functional>

template<typename ClassType, typename... Args>
class DelegateInstance
{

  public:
    using MethodPtr = void(ClassType::*)(Args...);

    DelegateInstance(ClassType* pObject, MethodPtr pMethod) : Object(pObject), Method(pMethod) {}

    void Execute(Args... pArgs)
    {
      
      if(Object && Method)
        (Object->*Method)(pArgs...);

    }

    void IsBoundTo(ClassType* pObject, MethodPtr pMethod) const
    {
    
      return Object == pObject && Method == pMethod;

    }

  private:
    ClassType* Object;
    MethodPtr Method;

};
