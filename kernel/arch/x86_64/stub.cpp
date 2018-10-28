#include <cstdint>
#include "../../stub.hpp"
// a printxy function to be used until terminal is initialized properly and we can implement a printf
void printxy(const char* text, int row, int column) {
    uint64_t offset = column + row * 80;
    asm (R"(
        mov %0, %%rcx
        mov %1, %%rdx
        movabs $_printat, %%rax
        call *%%rax
    )" : : "r" (text), "r" (offset) : "%rax", "%rcx", "%rdx" );
}

extern "C" {
    extern void * CTORS_BEGIN;
    extern void * CTORS_END;
}

void init_ctors() {
    for (auto c = &CTORS_BEGIN; c < &CTORS_END; c++) {
        auto constructor = reinterpret_cast<void(*)(void)>(*c);
        constructor();
    }
}