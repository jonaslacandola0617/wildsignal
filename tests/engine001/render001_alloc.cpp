#include <atomic>
#include <cstdlib>
#include <new>
namespace audit { std::atomic<bool> watching{false};std::atomic<int> allocations{0}; }
void* operator new(std::size_t n){if(audit::watching)++audit::allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p) noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t) noexcept{::operator delete(p);}
