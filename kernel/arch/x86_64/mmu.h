
#include <cstdint>

template<typename T, typename W>
inline T andnot(T val, W mask) {
    T _mask = mask;
    val |= _mask;
    val ^= _mask;
    return val;
}



class MMU {

    template<int level>
    struct alignas(8) PageEntry {
        uint64_t data = 0;

        void reset() {
            data = 0;
        }

        void set_addr (uint64_t addr) {
            uint64_t lsb = 1ull;
            lsb <<= (pagesize()?level * 9:9) + 3;
            data %= lsb;
            data |= (addr / lsb) * lsb;
        }
        uint64_t get_addr() {
            uint64_t lsb = 1ull;
            lsb <<= (pagesize()?level * 9:9) + 3;
            return (data / lsb) * lsb;
        }

        struct BitReference {
            BitReference(uint64_t &fieldref, int shift): ref{fieldref},mask{1ull<<shift} {}

            BitReference& operator = (bool value) {
                ref = value ? ref | mask : andnot(ref,mask);
                return *this;
            }

            operator bool () {
                return !! (ref & mask);
            }
        private:
            uint64_t &ref;
            uint64_t mask;
        };

        // pt pp
        auto present () {
            return BitReference(data, 0);
        }

        // pt pp
        auto writable() {
            return BitReference(data, 1);
        }

        // pt pp
        auto user_accessible() {
            return BitReference(data, 2);
        }

        // pt pp
        auto page_writethrough() {
            return BitReference(data, 3);
        }

        // pt pp
        auto page_disablecache() {
            return BitReference(data, 4);
        }

        // pt pp
        auto accessed() {
            return BitReference(data, 5);
        }

        // pp
        auto page_dirty() {
            return BitReference(data, 6);
        }

        // pt pp
        auto pagesize() {
            return BitReference(data, 7);
        }

        // pp
        auto global() {
            return BitReference(data, 8);
        }

        // pp
        auto pat() {
            if (level == 1)
                return BitReference(data, 7);
            else
                return BitReference(data, 12);
        }

        // 59-62 protection key (TODO)

        auto execute_disable() {
            return BitReference(data, 63);
        }
    };

public:
    struct PML4E: public PageEntry<4> {
    };
    struct PDPTE: public PageEntry<3> {
    };
    struct PDE: public PageEntry<2> {
        uint64_t translate(void* pointer) {
            uintptr_t uintp = reinterpret_cast<uint64_t>(pointer);
            uint64_t base = get_addr();
            return base + uintp & 0x1fffff;
        }
    };
    struct PTE: public PageEntry<1> {
        uint64_t translate(void* pointer) {
            uintptr_t uintp = reinterpret_cast<uint64_t>(pointer);
            uint64_t base = get_addr();
            return base + uintp & 0xfff;
        }
    };

    struct alignas(0x1000) PT {
        PTE entries[512];
    };

    struct alignas(0x1000) PDT {
        PDE entries[512];
    };

    struct alignas(0x1000) PDPT {
        PDPTE entries[512];
    };

    enum class MapResult {
        Ok = 0,
        AlreadyMapped = -1,
        NoTable = -2
    };

    struct alignas(0x1000) PML4T {
        PML4E entries[512];
        // will fail horribly if the target address space doesn't have
        // the same stack mapped in the same address
        void switchTo();
        MapResult mapTable(void* vaddr, int paddr, int level);
        MapResult mapPage(void* vaddr, int paddr, int level);
    };

    void init_kernel_vspace();
    PML4T* get_kernel_vspace();
    PDPTE get_kernel_vmap();

    // the stack used to call this function is expected to be mapped to the same address
    // in both vspaces and kernel (e.g., a kernel space stack)
};