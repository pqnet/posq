#include "mmu.h"

// symbols from linker. We only need their address
extern "C" {
    extern uint8_t BOOTSTRAP_END;
    extern uint8_t KERNEL_VIRTUAL_BASE;
    extern uint8_t KERNEL_VIRTUAL_BASE_END;
}

constexpr uint8_t L1LSB = 12;
constexpr uint8_t L2LSB = 21;
constexpr uint8_t L3LSB = 30;
constexpr uint8_t L4LSB = 39;

// Virtual address resolution
// 0-11 -> linear
// 12-20 -> PT offset/linear (2mb pages)
// 21-29 -> PDT offset/linear (1gb pages)
// 30-38 -> PDPT offset
// 39-47 -> PML4T offset
// 48-64 -> should sign extend bit 47
// Current address space map:
// 0x00000000 00000000 - 0x00007fff ffffffff  low memory (userspace)
// 0xffff8000 00000000 - 0xffffffff ffffffff  low memory (userspace)
// PML4T entries
// 0x00000000 00000000 - 0x0000003f ffffffff  entry 0
// 0x00000000 00000000 - 0x00007fff ffffffff  entry 255
// 0xffff8000 00000000 - 0xffff803f ffffffff  entry 256
// 0xffffffc0 00000000 - 0xffffffff ffffffff  entry 511
// we use entry 511 for kernel code and data

static MMU::PML4T* kernel_space; // a pointer to L4 table in linear space
static MMU::PML4T kernel_space_l4; // maps EVERYTHING
static MMU::PDPT kernel_space_l3; // maps the uppest 512GB of virtual space

// these define mappings for virtual addresses from kernel_virtual_base to the end of address space
static MMU::PDT kernel_space_l2[2]; // each entry maps 2M, each table maps 1G, total 2G

constexpr uint64_t MAX_PHYSADDR = (1ull<<L4LSB) - 1;
// linear address mapped from this pointer
const void* linear_address_base = reinterpret_cast<void*>(0xffff800000000000ull);

static MMU::PDPT linear_space_l3;
static MMU::PDT linear_space_l2[512]; // each entry maps 2M, each table maps 1G, total 512G

// from linear address space to physical address
inline uint64_t ltp(void* linearAddress) {
    return  reinterpret_cast<uint64_t>(linearAddress) & MAX_PHYSADDR;
}
// from physical address to linear mapped address
inline void* ptl(uint64_t physical_address) {
    return  ((char*)linear_address_base) + (physical_address & MAX_PHYSADDR);
}

// extern const uint64_t kernel_vtp_offset;
inline uint64_t ktp(uint64_t ptr) {
    uint64_t const kernel_virtual_base = reinterpret_cast<uint64_t>(&KERNEL_VIRTUAL_BASE);
    uint64_t const kernel_physaddr = reinterpret_cast<uint64_t>(&BOOTSTRAP_END);
    //const uint64_t kernel_vtp_offset = kernel_virtual_base - kernel_physaddr;
    return uint64_t{ptr - kernel_virtual_base + kernel_physaddr};
}
inline uint64_t ktp(void* ptr) {
    return ktp(reinterpret_cast<uint64_t>(ptr));
};

void MMU::init_kernel_vspace() {
    // we assume (for virtual to real address mapping) to be in bootstrap space:
    // - first 2mb+ of memory mapped linearly
    // - kernel located physically from kernel_physaddr to and mapped from
    // kernel_virtual_base to kernel_virtual_base_end

    uint64_t const kernel_virtual_base = reinterpret_cast<uint64_t>(&KERNEL_VIRTUAL_BASE);
    uint64_t const kernel_virtual_base_end = reinterpret_cast<uint64_t>(&KERNEL_VIRTUAL_BASE_END);

    // map kernel memory in 2mb pages
    for (auto addr = kernel_virtual_base; addr< kernel_virtual_base_end; addr+= 1<<L2LSB) {
        auto &l2entry = kernel_space_l2[(addr >> L3LSB) & 1].entries[(addr >> L2LSB) & 511];
        l2entry.pagesize() = true;
        l2entry.set_addr(ktp(addr));
        l2entry.present() = true;
        // this contains kernel stack, should be writable.
        l2entry.writable() = true;
    }
    for (auto addr = kernel_virtual_base; addr< kernel_virtual_base_end; addr+= 1<<L3LSB) {
        auto &l3entry = kernel_space_l3.entries[(addr >> L3LSB) & 511];
        l3entry.set_addr(ktp(&kernel_space_l2[(addr >> L3LSB) & 1]));
        l3entry.present() = true;
        l3entry.writable() = true;
    }
    
    // map 512 gb of physical memory linearly
    for (int i = 0; i < 512; i++) {
        for (int j = 0; j < 512; j++) {
            auto &l2entry = linear_space_l2[i].entries[j];
            l2entry.pagesize() = true;
            l2entry.set_addr((i<<L3LSB) + (j<<L2LSB));
            l2entry.present() = true;
            l2entry.writable() = true;
            l2entry.execute_disable() = true;
        }
        auto &l3entry = linear_space_l3.entries[i];
        l3entry.set_addr(ktp(&linear_space_l2[i]));
        l3entry.present() = true;
        l3entry.writable() = true;
        l3entry.execute_disable() = true;
    }
    // Temporary mapping to write into VGA. We will map VGA to a better address once this works
    {
        static MMU::PDT kernel_lowmeml2;
        static MMU::PDPT kernel_lowmeml3;

        auto &l4 = kernel_space_l4.entries[0];
        auto &l3 = kernel_lowmeml3.entries[0];
        auto &l2 = kernel_lowmeml2.entries[0];
        l4.set_addr(ktp(&kernel_lowmeml3));
        l3.set_addr(ktp(&kernel_lowmeml2));
        l2.set_addr(0);
        l2.pagesize() = true;
        l2.present() = true;
        l2.writable() = true;
        l3.present() = true;
        l3.writable() = true;
        l4.present() = true;
        l4.writable() = true;
    }
    
    auto & l4linearentry = kernel_space_l4.entries[256];
    auto & l4kernelentry = kernel_space_l4.entries[511];
    l4linearentry.set_addr(ktp(&linear_space_l3));
    l4linearentry.present() = true;
    l4linearentry.writable() = true;
    l4kernelentry.set_addr(ktp(&kernel_space_l3));
    l4kernelentry.present() = true;
    l4kernelentry.writable() = true;
    // save the linear space pointer to PML4T into kernel_space, for future reference
    uint64_t l4physaddr = ktp(&kernel_space_l4);
    kernel_space = static_cast<MMU::PML4T*>(ptl(l4physaddr));
    // switch to new address space
    // kernel_space->switchTo();
}
void MMU::PML4T::switchTo() {
    uint64_t physAddr = ltp(this);
    asm(R"(
        mov %0,%%cr3
    )"
        :
        : "r"(physAddr) );
}

MMU::PML4T* MMU::get_kernel_vspace() {
    return kernel_space;
}

MMU::PDPTE MMU::get_kernel_vmap() {
    uint64_t addr = reinterpret_cast<uint64_t>(&KERNEL_VIRTUAL_BASE);
    return kernel_space_l3.entries[(addr >> L3LSB) & 511];
}