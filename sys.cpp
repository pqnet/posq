#include "stub.hpp"

static inline int kmain(int argc, char const ** argv) {    
    return 0;
}

// entry point
extern "C" {
void _cstart() {
    // notify world we are running High Level 64-bit code
    printxy("Hello from C++64!", 10, 9);
}
}
