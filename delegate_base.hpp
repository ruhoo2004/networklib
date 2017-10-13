#pragma once
#include <memory>
#include <boost/variant.hpp>

template<typename T>
class delegate_base;

template<typename RET, typename ...PARAMS>
class delegate_base<RET(PARAMS...)>
{
protected:
    using stub_type = RET(*)(void* this_ptr, PARAMS...);

    struct InvocationElement {
        InvocationElement() = default;

        InvocationElement(void* this_ptr, stub_type aStub):
            object(this_ptr), shared(false), stub(aStub)
        {}

        InvocationElement(const std::shared_ptr<void>& this_ptr, stub_type aStub):
            object(this_ptr), shared(true), stub(aStub)
        {}

        InvocationElement(const InvocationElement& another):
            object(another.object),
            shared(another.shared),
            stub(another.stub)
        {
        }

        RET Invoke(PARAMS...args) const
        {
            if (shared) {
                return (*stub)(boost::get<std::shared_ptr<void>>(object).get(), args...);
            } else {
                return (*stub)(boost::get<void*>(object), args...);
            }
        }

        bool operator == (const InvocationElement& another) const
        {
            return another.stub == stub && another.object == object;
        }

        bool operator == (void* ptr) const {
            return stub == nullptr;
        }

        bool operator != (const InvocationElement& another) const
        {
            return another.stub != stub || !(another.object == object);
        }

        bool operator != (void* ptr) const {
            return stub != nullptr;
        }
private:
        boost::variant<void*, std::shared_ptr<void>> object;
        bool shared = false;
        stub_type stub = nullptr;
    };
};
