#include "stub.hpp"
#include "console.hpp"
#include "arch/x86_64/mmu.h"

static inline int kmain(int argc, char const ** argv) {    
    return 0;
}

// entry point
extern "C" {
void _cstart() {
    // notify world we are running High Level 64-bit code
    printxy("Hello from C++64!", 10, 9);
    console.initialize();
    console.disableCursor();
    console.enableCursor();
    console.moveCursor(0,0);
    init_ctors();
    for (int i = 0; i < 25; i++) {
        console.writeString("0x");
        console.writeNumber(i, 2, 16);
        console.writeString(" ");
        console.writeNumber(i);
        console.writeString("     \n");
    }
    console.clearScreen();
    console.moveCursor(0,5);
    console.printf("puppa col sushi\n");
    console.printf("puppa col sushi %s\n", "perdavvero");
    console.printf("puppa col sushi %s %d volte\n", "perdavvero", 3);
    console.printf("About to initialize the new page table structures\n");
    MMU mmu;
    mmu.init_kernel_vspace();
    console.printf("About to switch to the new page table structures. Wish me good luck\n");

    mmu.get_kernel_vspace()->switchTo();
    console.printf("Apparently stack is still good after switching to new page tables. Yay!\n");
    
    // initialize proper terminal and early logging facilities
    // initialize memory manager (allocator)
    // initialize ipc
    // load system suite processes (drivers)

}
}
