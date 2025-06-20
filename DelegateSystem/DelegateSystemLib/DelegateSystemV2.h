/*
* UE5's Delegates System Replication
* Copyright Javier Benito Abolafio
*/

#include <vector>

/*
* Delegate Interface to allow polymorphism
*/
template <typename... Args>
class IDelegate
{
  public:
    virtual ~IDelegate() = default;
    virtual void Execute(Args... args) = 0;
    virtual const std::type_info& GetObjectType() const = 0;

};

/*
* Delegate Instance
* This represents the delegate itself, a class which will store the callback and its owning class
*/
template <typename ClassType, typename... Args>
class DelegateInstance : public IDelegate<Args...>
{
  public:
    using MethodPtr = void (ClassType::*)(Args...);

    DelegateInstance(ClassType* obj, MethodPtr ptr) : _Object(obj), _Method(ptr) {}

    void Execute(Args... args) override
    {
      if(_Object && _Method)
        (_Object->*_Method)(args...);
    }

    template <typename T>
    bool IsBound(T* obj, void(T::*method)(Args...)) const
    {
      if (nullptr == obj || nullptr == method)
        return false;

      return (_Object == obj && _Method == reinterpret_cast<MethodPtr>(method));
    }

    const std::type_info& GetObjectType() const override
    {
      return typeid(ClassType);
    }

  private:
    MethodPtr _Method = nullptr;
    ClassType* _Object;
};

template <typename... Args>
class MulticastDelegate
{
  public:
    template <typename ClassType>
    void Add(ClassType* obj, void(ClassType::*ptr)(Args...))
    { 
      if(IsBound(obj, ptr))
        return;

      _vDelegates.push_back(new DelegateInstance<ClassType, Args...>(obj, ptr));
    }

    void Broadcast(Args... args) {}

  private:
    template <typename ClassType>
    bool IsBound(ClassType* obj, void(ClassType::*ptr)(Args...))
    {
      for(auto* delegate : _vDelegates)
      {
        if(delegate->GetObjectType() == typeid(ClassType))
        {
          auto* typedDelegate = static_cast<DelegateInstance<ClassType, Args...>*>(delegate);
          if(typedDelegate->IsBound(obj, ptr)) return true;
        }
      }
      return false;
    }

  private:
    std::vector<IDelegate<Args...>*> _vDelegates;

};

/*
* TEST CASE
* ----------
* DelegateInstance<param> myDelegate;
* myDelegate.Add();
*/
