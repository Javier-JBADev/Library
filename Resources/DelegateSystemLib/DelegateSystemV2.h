/*
* UE5's Delegates System Replication
* Copyright Javier Benito Abolafio
*/

/*
* DelegateName: Define the name of the delegate
* Append as many args as needed
* E.g. DECLARE_DYNAMIC_MULTICAST(OnHit, float)
*/ 
#define DECLARE_DYNAMIC_MULTICAST(DelegateName, ...) \
class DelegateName : public MulticastDelegate<__VA_ARGS__> {};

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
* Callback struct
* This offers another way of creating, adding and removing callbacks
* E.g. Callback<Class, params> newCallback(Class, Method)
* delegate += newCallback
*/
template <typename ClassType, typename... Args>
class Callback
{
  public:
    using MethodPtr = void (ClassType::*)(Args...);

    Callback(ClassType* obj, MethodPtr method) : _Method(method), _Object(obj) {}
    MethodPtr _Method = nullptr;
    ClassType* _Object;
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
    ~DelegateInstance() = default;

    void Execute(Args... args) override
    {
      if(!_Object || !_Method) return;

      (_Object->*_Method)(args...);
    }

    template <typename T>
    bool IsBound(T* obj, void(T::*method)(Args...)) const
    {
      if (nullptr == obj || nullptr == method)  return false;

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
    ~MulticastDelegate()
    {
      for(auto* delegate : _vDelegates)
      {
        delete delegate;
      }
      _vDelegates.clear();
    }

  /*
  * Override Operators
  * += to Add
  * -= to Remove
  */
  public:
    template <typename ClassType>
    MulticastDelegate& operator += (const Callback<ClassType, Args...>& callback)
    {
      Add(callback._Object, callback._Method);
      return *this;
    }

    template <typename ClassType>
    MulticastDelegate& operator -= (const Callback<ClassType, Args...>& callback)
    {
      Remove(callback._Object, callback._Method);
      return *this;
    }

  public:
    template <typename ClassType>
    void Add(ClassType* obj, void(ClassType::*ptr)(Args...))
    { 
      if(IsBound(obj, ptr)) return;

      _vDelegates.push_back(new DelegateInstance<ClassType, Args...>(obj, ptr));
    }

    template <typename ClassType>
    void Remove(ClassType* obj, void(ClassType::*ptr)(Args...))
    {
      auto it = std::find_if(_vDelegates.begin(), _vDelegates.end(), [obj, ptr](IDelegate<Args...>* delegate)
      {
        auto* typed = dynamic_cast<DelegateInstance<ClassType, Args...>*>(delegate);
        return typed && typed->IsBound(obj, ptr);
      });

      if(it != _vDelegates.end())
      {
        delete *it;
        _vDelegates.erase(it);
      }
    }

    void Broadcast(Args... args)
    {
      for(auto delegate : _vDelegates)
      {
        delegate->Execute(args...);
      }
    }

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
