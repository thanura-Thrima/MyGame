#pragma once
#include <memory>

template<class T>
class ThreadSafeHandle
{
public:
    ThreadSafeHandle(const std::shared_ptr<T>& handle): m_Handle(handle)
    {
    }
    ~ThreadSafeHandle() = default;
    ThreadSafeHandle(const ThreadSafeHandle&) = default;
    ThreadSafeHandle& operator=(const ThreadSafeHandle&) = default;
    ThreadSafeHandle(ThreadSafeHandle&&) = default;
    ThreadSafeHandle& operator=(ThreadSafeHandle&&) = default;
    static std::shared_ptr<T> makeNonOwningSharedPtr(T* ptr)
    {
        return std::shared_ptr<T>(ptr, [](T* ptr) {});
    }

    std::shared_ptr<T> lock() const
    {
        auto shared = m_Handle.lock();
        if (!shared) {
            return nullptr;
        }

        if (!shared->threadAssigned()) {
            return nullptr;
        }

        return shared;
    }

private:
    std::weak_ptr<T> m_Handle;
};