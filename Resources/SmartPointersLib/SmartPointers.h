#pragma once

#include <iostream>
#include <mutex>

namespace JBA
{
  /// unique_ptr
  template <typename T>
  class unique_ptr
  {
    private:
      T* _ptr;

    public:
      explicit unique_ptr(T* p = nullptr) : _ptr(p) {}
      ~unique_ptr() { delete _ptr; }

      /*
      * Delete copy constructor and = operator
      */
      unique_ptr(const unique_ptr&) = delete;
      unique_ptr& operator=(const unique_ptr&) = delete;

      /*
      * Move constructor
      */
      unique_ptr(unique_ptr&& other) noexcept : _ptr(other._ptr)
      {
        other._ptr = nullptr;
      }

      /*
      * Move assignment
      */
      unique_ptr& operator=(unique_ptr&& other) noexcept
      {
        if(this != &other)
        {
          delete _ptr;
          _ptr = other._ptr;
          other._ptr = nullptr;

        }
        return *this;
      }

      T* get() const { return _ptr; }
      T& operator*() const { return *_ptr; }
      T* operator->() const { return _ptr; }

      T* release()
      {
        T* tempPtr = _ptr;
        _ptr = nullptr;
        return tempPtr;
      }

      void reset(T* ptr)
      {
        delete _ptr;
        _ptr = ptr;
      }

  };

  /// shared_ptr block
  template <typename T>
  class SharedControlBlock
  {
    public:
      SharedControlBlock(T* ref = nullptr) : _ref(ref), _refCounter(0) {};

    public:
      T* _ref;
      size_t _refCounter;
      std::mutex _mutex;
  };

  /// shared_ptr
  template <typename T>
  class shared_ptr
  {
    private:
      SharedControlBlock<T>* _ptr;

    public:
      shared_ptr() : _ptr(nullptr) {}

      explicit shared_ptr(T* p)
      {
        _ptr = new SharedControlBlock<T>(p);
        std::lock_guard<std::mutex> lock(_ptr->_mutex);
        ++_ptr->_refCounter;          
      }

      explicit shared_ptr(SharedControlBlock<T>* sharedBlock) : _ptr(sharedBlock)
      {
        if(!_ptr)
          return;

        std::lock_guard<std::mutex> lock(_ptr->_mutex);
        ++_ptr->_refCounter;
      }

      ~shared_ptr()
      {
        if(!_ptr)
          return;
        
        std::lock_guard<std::mutex> lock(_ptr->_mutex);
        --_ptr->_refCounter;
        if(_ptr->_refCounter == 0)
        {
          delete _ptr->_ref;
          delete _ptr;
        }
      }

      shared_ptr(const shared_ptr& other)
      {
        _ptr = other._ptr;
        if(!_ptr)
          return;

        std::lock_guard<std::mutex> lock(_ptr->_mutex);
        ++_ptr->_refCounter;
      }

      shared_ptr& operator=(const shared_ptr& other)
      {
        if(this == &other)
          return *this;
        
        if(nullptr != _ptr)
        {
          std::lock_guard<std::mutex> lock(_ptr->_mutex);
          --_ptr->_refCounter;
          if(_ptr->_refCounter == 0)
          {
            delete _ptr->_ref;
            delete _ptr;
          }
        }

        _ptr = other._ptr;

        if(_ptr)
        {
          std::lock_guard<std::mutex> lock(_ptr->_mutex);
          ++_ptr->_refCounter;
        }

        return *this;

      }

      T* get() const { return _ptr ? _ptr->_ref : nullptr; }
      T& operator*() const { return *_ptr->_ref; }
      T* operator->() const { return _ptr->_ref; }
      
  };

  /*
  * Generic functions to create and initialise smart pointers
  */
  template <typename T, typename... Args>
  unique_ptr<T> make_unique(Args... args)
  {
    unique_ptr<T> ptr(new T(std::forward<Args>(args)...));
    return ptr;
  }

  template <typename T, typename... Args>
  shared_ptr<T> make_shared(Args&&... args)
  {
    SharedControlBlock<T>* block = new SharedControlBlock<T>(new T(std::forward<Args>(args)...));
    return shared_ptr<T>(block);
  }

}

/*
* --- API DESIGN ---
* JBA::unique:ptr<type> _ptrName;
* JBA::unique_ptr<type> _ptrName = JBA::make_unique<type>();
* 
* type* _ptrName = new Type() -> JBA::unique_ptr<type> _ptrName = JBA::unique_ptr<type>();
* 
* JBA::shared_ptr<type> _ptrName()
*/