#pragma once
#include "delegate_base.hpp"
#include <list>

template<typename T> class delegate;
template<typename T> class multicast_delegate;

template<typename RET, typename ...PARAMS>
class delegate<RET(PARAMS...)> final: private delegate_base<RET(PARAMS...)>
{
public:
    delegate() = default;

    delegate(const delegate& another) {
        for (auto& element: another.invocationList) {
            invocationList.emplace_back(new IElement(*element.get()));
        }
    }

    delegate& operator =(const delegate& another) {
        invocationList.clear();

        for (auto& element: another.invocationList) {
            invocationList.emplace_back(new IElement(*element.get()));
        }
        return *this;
    }

    delegate& operator =(void* ptr) {
        invocationList.clear();
        return *this;
    }

    template<typename ReturnType = RET>
    typename std::enable_if_t<!std::is_void<ReturnType>::value, RET>
    operator()(PARAMS... arg) const {
        RET ret;
        for (auto& element: invocationList) {
            ret = element->Invoke(arg...);
        }
        return ret;
    }

    template<typename ReturnType = RET>
    typename std::enable_if_t<std::is_void<ReturnType>::value, void>
    operator()(PARAMS... arg) const {
        for (auto& element: invocationList) {
            element->Invoke(arg...);
        }
    }

    bool operator == (void* ptr) const {
        return (ptr == nullptr) && (invocationList.empty());
    }
    bool operator == (const delegate& another) const {
        return std::equal(invocationList.begin(), invocationList.end(),
                another.invocationList.begin(),
                [](const std::unique_ptr<IElement>& a,
                   const std::unique_ptr<IElement>& b) {return *a == *b;});
    }

    bool operator != (void* ptr) const {
        return (ptr != nullptr) || (!invocationList.empty());
    }

    bool operator != (const delegate& another) const {
        return !(*this == another);
    }

    delegate& operator += (const delegate& another) {
        if (another == nullptr) return *this;
        for (auto& element: another.invocationList) {
            invocationList.emplace_back(new IElement(*element.get()));
        }
        return *this;
    }

    delegate& operator -= (const delegate& another) {
        auto ait = another.invocationList.end();
        while (ait != another.invocationList.begin()) {
            ait--;
            auto it = invocationList.end();
            while (it != invocationList.begin()) {
                it--;
                if (**ait == **it) {
                    it = invocationList.erase(it);
                    break;
                }
            }
        }
        return *this;
    }

    template<typename LAMBDA>
    delegate& operator += (const LAMBDA& instance) {
        invocationList.emplace_back(new IElement((void*)(&instance), lambda_stub<LAMBDA>));
        return *this;
    }
    template<typename LAMBDA>
    delegate& operator -= (const LAMBDA& instance) {
        auto tmp = std::make_unique<IElement>((void*)(&instance), lambda_stub<LAMBDA>);
        auto it = invocationList.end();
        while (it != invocationList.begin()) {
            it--;
            if (**it == *tmp) {
                invocationList.erase(it);
                break;
            }
        }
        return *this;
    }

    template<typename LAMBDA>
    static delegate create(const LAMBDA& instance) {
        return delegate((void*)(&instance), lambda_stub<LAMBDA>);
    }

    template<class T, RET(T::*TMethod) (PARAMS...)>
    static delegate create(T* instance) {
        return delegate(instance, method_stub<T, TMethod>);
    }

    template<class T, RET(T::*TMethod) (PARAMS...) const>
    static delegate create(T* instance) {
        return delegate(instance, method_stub<T, TMethod>);
    }

    template<class T, RET(T::*TMethod) (PARAMS...)>
    static delegate create(const std::shared_ptr<T>& instance) {
        return delegate(instance, method_stub<T, TMethod>);
    }

    template<class T, RET(T::*TMethod) (PARAMS...) const>
    static delegate create(const std::shared_ptr<T>& instance) {
        return delegate(instance, method_stub<T, TMethod>);
    }

    template<RET(*TMethod) (PARAMS...)>
    static delegate create() {
        return delegate(nullptr, function_stub<TMethod>);
    }

private:
    delegate(void* anObject, typename delegate_base<RET(PARAMS...)>::stub_type aStub) {
        invocationList.emplace_back(new IElement(anObject, aStub));
    }

    delegate(const std::shared_ptr<void>& anObject, typename delegate_base<RET(PARAMS...)>::stub_type aStub) {
        invocationList.emplace_back(new IElement(anObject, aStub));
    }
    
    template<class T, RET(T::*TMethod)(PARAMS...)>
    static RET method_stub(void* this_ptr, PARAMS... params) {
        T* p = static_cast<T*>(this_ptr);
        return (p->*TMethod)(params...);
    }

    template<class T, RET(T::*TMethod)(PARAMS...) const>
    static RET method_stub(void* this_ptr, PARAMS... params) {
        T* p = static_cast<T*>(this_ptr);
        return (p->*TMethod)(params...);
    }

    template<RET(*TMethod)(PARAMS...)>
    static RET function_stub(void* this_ptr, PARAMS... params) {
        return (*TMethod)(params...);
    }

    template<typename LAMBDA>
    static RET lambda_stub(void* this_ptr, PARAMS... arg) {
        LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
        return (p->operator())(arg...);
    }

private:
    using IElement = typename delegate_base<RET(PARAMS...)>::InvocationElement;
    std::list<std::unique_ptr<IElement>> invocationList;

};
