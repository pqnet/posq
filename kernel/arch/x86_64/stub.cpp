#include <cstdint>
#include "../../stub.hpp"
// a printxy function to be used until terminal is initialized properly and we can implement a printf
void printxy(const char* text, int row, int column) {
    uint64_t offset = column + row * 80;
    asm (R"(
        push %%rax
        push %%rcx
        push %%rdx
        mov %0, %%rcx
        mov %1, %%rdx
        call _printat
        pop %%rdx
        pop %%rcx
        pop %%rax
    )" : : "r" (text), "r" (offset));
}