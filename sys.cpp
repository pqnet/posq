#include <cstdint>

static inline int kmain(int argc, char const ** argv) {
    return 0;
}

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

// entry point
extern "C" {
void _cstart() {
    printxy("Hello from C!", 10, 9);
}
}
