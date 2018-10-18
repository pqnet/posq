
#include "buddy.hpp"

// thread allocator
thread_local void* (*thread_malloc)();
thread_local void* (*thread_free)();

// a simple memory allocator which can handle using standard library in bootstrap tasks.
// W
template<class T>
struct simple_allocator {
    using value_type = T;
    simple_allocator() noexcept;
    template <class U> simple_allocator (const simple_allocator<U>&) noexcept;
    T* allocate(std::size_t n);
    void deallocate(T* p, std::size_t n);
};

template <class T, class U>
constexpr bool operator== (const simple_allocator<T>&, const simple_allocator<U>&) noexcept;

template <class T, class U>
constexpr bool operator!= (const simple_allocator<T>&, const simple_allocator<U>&) noexcept;