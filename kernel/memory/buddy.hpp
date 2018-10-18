#include <bitset>
#include <array>
#include <atomic>

// a tree structure of buddies, with a nice square packing
// To guarantee packing, each level should be at least 64bit = 2^6bit big
//  hence buddy_node_levels >= 5.
template<int buddy_node_levels> struct buddy_leaf {
    static constexpr size_t buddy_size = 2 * buddy_node<buddy_node_levels-1>::buddy_size;

    buddy_node<buddy_node_levels-1> upper;
    std::bitset< 1<<buddy_node_levels > buddies;

    template<int level>
    bool add_memory_real(size_t pos) {
        return upper.add_memory_real(pos - 1);
    }
    template<int level>
    bool remove_memory_real(size_t pos) {
        return upper.remove_memory_real(pos - 1);
    }
    template<int level>
    ptrdiff_t find_memory_real(size_t pos_start, size_t pos_end) {
        return upper.find_memory_real(pos_start, pos_end) ;
    }

    template<>
    bool add_memory_real<0>(size_t pos) {
        bool ret = !buddies.test(pos);
        buddies.set(pos);
        return ret;
    }
    template<>
    bool remove_memory_real<0>(size_t pos) {
        bool ret = buddies.test(pos);
        buddies.reset(pos);
        return ret;
    }
    template<>
    ptrdiff_t find_memory_real<0>(size_t pos_start, size_t pos_end) {
        if (buddies.none()) {
            return -1;
        }
        for (size_t i = pos_start; i< pos_end; i++) {
            if (buddies.test(i)) {
                return i;
            }
        }
        return -1;
    }

        
    void lock () {
        upper.lock();
    }

    void unlock () {
        upper.unlock()
    }
};

// top level of the buddy. Also the smallest buddy leaf possible
template<> struct buddy_leaf<5> {
    static constexpr size_t buddy_size = 32;

    // use layout p 5 2x4 4x3 8x2 16x1 32x0
    struct buddy_min_leaf_impl {
        int _lock  :1;
        int level5 :1;
        int level4 :2;
        int level3 :4;
        int level2 :8;
        int level1 :16;
        int level0 :32;

        template<int level>
        bool add_memory_real(size_t pos) {
            if constexpr (level > 5) {
                return false
            } else if constexpr (level == 5) {
                bool rval = !(level5 & 1 << pos);
                level5 |= 1<<pos;
                return rval;
            } else if constexpr (level == 4) {
                bool rval = !(level4 & 1 << pos);
                level4 |= 1<<pos;
                return rval;
            } else if constexpr (level == 3) {
                bool rval = !(level3 & 1 << pos);
                level3 |= 1<<pos;
                return rval;
            } else if constexpr (level == 2) {
                bool rval = !(level2 & 1 << pos);
                level2 |= 1<<pos;
                return rval;
            } else if constexpr (level == 1) {
                bool rval = !(level1 & 1 << pos);
                level1 |= 1<<pos;
                return rval;
            } else if constexpr (level == 0) {
                bool rval = !(level0 & 1 << pos);
                level0 |= 1<<pos;
                return rval;
            }
        }

        template<int level>
        bool remove_memory_real(size_t pos) {
            if constexpr (level > 5) {
                return false
            } else if constexpr (level == 5) {
                bool rval = !!(level5 & 1 << pos);
                level5 &= ~(1<<pos);
                return rval;
            } else if constexpr (level == 4) {
                bool rval = !!(level4 & 1 << pos);
                level4 &= ~(1<<pos);
                return rval;
            } else if constexpr (level == 3) {
                bool rval = !!(level3 & 1 << pos);
                level3 &= ~(1<<pos);
                return rval;
            } else if constexpr (level == 2) {
                bool rval = !!(level2 & 1 << pos);
                level2 &= ~(1<<pos);
                return rval;
            } else if constexpr (level == 1) {
                bool rval = !!(level1 & 1 << pos);
                level1 &= ~(1<<pos);
                return rval;
            } else if constexpr (level == 0) {
                bool rval = !!(level0 & 1 << pos);
                level0 &= ~(1<<pos);
                return rval;
            }
        }

        template<int level>
        ptrdiff_t find_memory_real(size_t pos_start, size_t pos_end) {
            int scan = 0;
            switch (level) {
                case 0: scan = level0; break;
                case 1: scan = level1; break;
                case 2: scan = level2; break;
                case 3: scan = level3; break;
                case 4: scan = level4; break;
                case 5: scan = level5; break;
            }
            scan =>> pos_start;
            if (scan == 0) {
                return -1;
            } else {
                int lssb = pos_start;
                while (scan % 1 == 0 && lssb < pos_end) {
                    lssb++;
                    scan /= 2;
                }
                if (lssb < pos_end) {
                    return lssb;
                } else {
                    return -1;
                }
            }
        }
    };

    std::atomic<buddy_min_leaf_impl> locked;

    template<int level>
    bool add_memory_real(size_t pos) {
        auto bucket = locked.load(std::memory_order_relaxed);
        bucket.add_memory_real<level>(pos);
        locked.store(bucket, std::memory_order_relaxed);
    }

    template<int level>
    bool remove_memory_real(size_t pos) {
        auto bucket = locked.load(std::memory_order_relaxed);
        bucket.remove_memory_real<level>(pos);
        locked.store(bucket, std::memory_order_relaxed);
    }

    template<int level>
    ptrdiff_t find_memory_real(size_t pos_start, size_t pos_end) {
        auto bucket = locked.load(std::memory_order_relaxed);
        return bucket.find_memory_real<level>(pos_start, pos_end);
    }
    
    void lock () {
        buddy_min_leaf_impl bucket, b2;
        do {
            bucket = locked.load(std::memory_order_relaxed);
            b2 = bucket;
            b2._lock = 1;
        } while (bucket._lock || !locked.compare_exchange_weak(bucket, b2, std::memory_order_acquire));
    }

    void unlock () {
        auto bucket = locked.load(std::memory_order_relaxed);
        bucket._lock = 0;
        locked.store(bucket, std::memory_order_release);
    }

};

template<size_t branching, unsigned int level, size_t leaf_levels> struct buddy_node {

    static constexpr size_t lower_buddy_size = buddy_node<branching, level - 1, leaf_levels>::buddy_size;
    static constexpr size_t buddy_size = branching * lower_buddy_size;

    struct buddy_line {
        buddy_node<branching, level - 1, leaf_levels>* buddy;
        size_t atomIdx;
    }
    std::array<buddy_line, branching> branches;

    template<int level>
    bool add_memory_real(size_t pos) {
        return upper.add_memory_real(pos - 1);
    }

    template<int level>
    bool remove_memory_real(size_t pos) {
        return upper.remove_memory_real(pos - 1);
    }
    template<int level>
    ptrdiff_t find_memory_real(size_t pos_start, size_t pos_end) {
        return upper.find_memory_real(pos_start, pos_end) ;
    }

};

// total size = branching * (sizeof(T_address) + sizeof(buddy_leaf*)). 256 fits in a 4k page
// We also retain mapping information in order to be able to unmap/remap the branches.
// address is the number of the page relative to range.
// The pointer is null if the branch isn't mapped
template<size_t branching, size_t leaf_levels> struct buddy_node<branching, 0, leaf_levels> {
    static constexpr size_t leaf_buddy_size = buddy_leaf<leaf_levels>::buddy_size;
    static constexpr size_t branch_buddy_size = leaf_buddy_size;
    static constexpr size_t buddy_size = branching * branch_buddy_size;

    typedef buddy_leaf<leaf_levels> branch_type;

    struct buddy_line {
        branch_type* buddy;
        size_t atomIdx;
    }

    std::array<buddy_line, branching> branches;

    // start and end are in 2<<level units
    template<int level>
    void add_memory_inleaf(branch_type& leaf, size_t start, size_t end) {
        if (start % 2 == 1) {
            if (leaf.remove_memory_real<level>(start - 1)) {
                start--;
            } else {
                leaf.add_memory_real<level>(start);
            }
        }

        if (end % 2) {
            if (remove_memory_real<level>(end)) {
                end++;
            } else {
                add_memory_real<level>(end - 1);
            }
        }

        if constexpr (level < leaf_levels) {
            auto sstart = (start+1) / 2;
            auto send = end / 2;
            if (sstart != send) {
                add_memory_inleaf<level + 1> (leaf, sstart, send);
            }
        }
    }
    // start and end are in 2<<level units
    template<int level>
    void remove_memory_inleaf(branch_type& leaf, size_t start, size_t end) {
        if (start % 2 == 1) {
            // if we don't have a single unit in this level we will take a bigger unit in next level, and restore one to this level
            if (!leaf.remove_memory_real<level>(start)) {
                leaf.add_memory_real<level>(--start);
            }
        }

        if (end % 2) {
            // if we don't have a single unit in this level we will take a bigger unit in next level, and restore one to this level
            if (!leaf.remove_memory_real<level>(end - 1)) {
                leaf.add_memory_real<level>(++end);
            }

        }

        if constexpr (level < leaf_levels) {
            auto sstart = (start+1) / 2;
            auto send = end / 2;
            if (sstart != send) {
                remove_memory_inleaf<level + 1> (leaf, sstart, send);
            }
        }
    }

    template<int level>
    ptrdiff_t find_memory_inleaf(branch_type& leaf, size_t start, size_t end, size_t required_size) {
        ptrdiff_t found = -1;
        if (required_size == 1) {
            found = leaf.find_memory_real(start, end);
        }
        if constexpr (level < leaf_levels) {
            auto sstart = (start + 1) / 2;
            auto send = end / 2;
            auto srequired_size = (required_size + 1)/ 2;
            if (found == -1) {
                found = find_memory_inleaf<level + 1>(leaf, sstart, send, srequired_size)
            }
        }
        return found;
    }

    template<typename fmap_t, typename falloc_t>
    branch_type& map_branch(int branchIdx, fmap_t mmap, falloc_t mmalloc) {
        if(branches[branchIdx].atomIdx == 0) {
            branches[branchIdx].atomIdx = mmalloc(sizeof(branch_type));
            branches[branchIdx].buddy = mmap(branches[branchIdx].atomIdx, sizeof(branch_type));
            new (branches[branchIdx].buddy) branch_type();
        }
        if (branches[branchIdx].buddy == 0) {
            branches[branchIdx].buddy = mmap(branches[branchIdx].atomIdx, sizeof(branch_type));
        }
        return *branches[branchIdx].buddy;
    }

    template<typename funmap_t>
    void unmap_branch(int branchIdx, funmap_t munmap) {
        munmap(branches[branchIdx].atomIdx, sizeof(branch_type));
    }

    template<typename fmap_t, typename funmap_t, typename ffree_t>
    void prune_branch(int branchIdx, fmap_t mmap, funmap_t munmap, ffree_t mfree) {
        map_branch(branchIdx).~branch_type();
        unmap_branch(branchIdx);
        mfree(branches[branchIdx].atomIdx, sizeof(branch_type));
    }

    template<typename fmap_t, typename funmap_t, typename falloc_t> 
    void add_memory(size_t start, size_t end, fmap_t mmap, funmap_t munmap, falloc_t mmalloc) {
        auto soffset = start % branch_buddy_size;
        auto eoffset = end % branch_buddy_size;
        auto sstart = (start + branch_buddy_size - 1) / branch_buddy_size;
        auto send = end / branch_buddy_size;
        if (sstart > send) {
            add_memory_inleaf<0>(map_branch(send, mmap, mmalloc), soffset, eoffset);
        } else  {
            if (soffset != 0) {
                add_memory_inleaf<0>(map_branch(sstart-1, mmap, mmalloc), soffset, branch_buddy_size)
            }
            if (eoffset != 0) {
                add_memory_inleaf<0>(map_branch(send, mmap, mmalloc), 0, eoffset)
            }
    
            for (int i = sstart; i < send; i++) {
                add_memory_inleaf<0>(map_branch(i, mmap, mmalloc), 0, branch_buddy_size);
            }
        }
    }

    template<typename fmap_t, typename funmap_t, typename ffree_t> 
    void remove_memory(size_t start, size_t end, fmap_t mmap, funmap_t munmap, ffree_t mfree) {
        auto soffset = start % branch_buddy_size;
        auto eoffset = end % branch_buddy_size;
        auto sstart = (start + branch_buddy_size - 1) / branch_buddy_size;
        auto send = end / branch_buddy_size;
        if (sstart > send) {
            remove_memory_inleaf<0>(map_branch(send, mmap, nullptr), soffset, eoffset);
        } else  {
            if (soffset != 0) {
                remove_memory_inleaf<0>(map_branch(sstart-1, mmap, nullptr), soffset, branch_buddy_size)
            }
            if (eoffset != 0) {
                remove_memory_inleaf<0>(map_branch(send, mmap, nullptr), 0, eoffset)
            }
    
            for (int i = sstart; i < send; i++) {
                prune_branch(i, mmap, munmap, mfree);
            }
        }
    }

    template<typename fmap_t, typename funmap_t>
    ptrdiff_t find_memory(size_t start, size_t end, size_t required_size, fmap_t mmap, funmap_t munmap) {
        for (int i = 0; i < branching; i++) {
            if (branches[i].atomIdx) {
                auto found = find_memory_inleaf<0>(map_branch(i, fmap_t, nullptr), start, end, required_size);
                if (found !== -1) {
                    return found;
                }
            }
        }
        return -1;
    }
};

// yields the maximum levels of buddies that can be put in an atom for leaf duty
constexpr size_t buddy_leaf_max_levels(size_t byte_atom_size) {
    if (byte_atom_size == 1) {
        // we have a total of 8 bits in the buddy.
        // 4 of them are for first level, 2 for second level, and 1 for third level.
        // Total levels allocable = 3
        return 3;
    }
    /*
      else if (byte_atom_size == 8) {
        return 6;
    } else if (byte_atom_size == 0x1000)  { // 4k 
        return 15;
    } else if (byte_atom_size == 0x200000)  { // 2M 
        return 24;
    }*/
    return 1 + buddy_leaf_max_levels(byte_atom_size>>1);
}

constexpr size_t buddy_level0_size(size_t level_count) {
    /*
    if (level_count == 6) { // atom = 64bit
        return 0x20; // 32 pages
    } else if (level_count == 15) { //atom = 4k
        return 0x4000; // 16k pages
    } else if (level_count == 24) {
        return 0x800000; // 8M pages
    }
    */
    return (1<<level_count) >> 1;
}

// extended memory manager is always self-hosted.
template<size_t leaf_size, size_t node_size, size_t page_size>
class extended_memory_manager {
    typedef size_t address_type;
    // 2M nodes branch 128k times
    // 0x200000 / 0x10 = 0x20000 = 128k
    // 4k nodes branch 256 times
    // 0x1000 / 0x10 = 0x100 = 256
    constexpr static size_t branching = page_size / ( sizeof(void*) + sizeof(size_t));
    // Node | Leaf | Total number of allocable bits | Max memory (4k pages)
    //   2M |   2M | 0x10000000000 (1E12)           | plenty
    //   4k |   2M | 0x80000000 (2E9)               | 8 TB
    //  128 |   2M | 0x4000000 (64E6)               | 256 GB
    // 4k^2 |   4k | 0x40000000 (1E9)               | 4 TB
    //   4k |   4k | 0x400000 (4E6)                 | 16 GB
    //  256 |   4k | 0x100000 (1E6)                 | 4 GB

    // only support 1 level for now
    buddy_node<branching, 0, buddy_leaf_max_levels(leaf_size)> buddy_root;

    // optimize search by marking min and max addresses of memory added up until now
    address_type min_addr = ~0;
    address_type max_addr = 0;

    // low level functions to mark free memory and used memory

    // Mark memory as free
    template<typename fmap_t, typename funmap_t>
    void add_memory(address_type start, address_type end, fmap_t mmap, funmap_t munmap) {
        min_addr = std::min(min_addr, start);
        max_addr = std::max(max_addr, end);
        int count = start;
        auto tmpalloc = [&](size_t requested_size){
            ptrdiff_t memory = find_memory(min_addr, max_addr, requested_size);
            if (memory == -1) {
                memory = count;
                count += (requested_size + page_size - 1) / page_size;
            }
            return memory;
        };
        buddy_root.add_memory(start, end, mmap, munmap, tmpalloc);
        buddy_root.remove_memory(start, count, mmap, munmap, [](size_t){});
    }

    // Mark memory as used
    template<typename fmap_t, typename funmap_t>
    void remove_memory(address_type start, address_type end, fmap_t mmap, funmap_t munmap) {
        buddy_root.remove_memory(start, end, mmap, munmap);
    }

    // search for a block of required size starting from given address
    template<typename fmap_t, typename funmap_t>
    ptrdiff_t find_memory(address_type search_start, address_type search_end, size_t required_size, fmap_t mmap, funmap_t munmap) {
        return buddy_root.find_memory(search_start, search_end , required_size, mmap, munmap);
    }
};

// can manage any amount of memory, wastes 2Mb of kernel memory + 2M of extended memory
typedef extended_memory_manager<0x200000, 0x200000, 1> plenty_memory_manager;

// can manage up to 8TB of memory, wastes 4kb of kernel memory + 2M of extended memory
typedef extended_memory_manager<0x200000, 0x1000, 1> server_memory_manager;

// can manage up to 256GB of memory, wastes 128 bytes of kernel memory + 2M of extended memory
typedef extended_memory_manager<0x200000, 128, 1> desktop_memory_manager;

// can manage up to 16GB of memory, wastes 4kb of kernel memory + 4k of extended memory, max alloc = 16MB
typedef extended_memory_manager<0x200000, 0x1000, 1> cheap_memory_manager;

// can manage up to 4GB of memory, wastes 256 bytes of kernel memory + 4k of extended memory, max alloc = 16MB
typedef extended_memory_manager<0x200000, 0x1000, 1> tiny_memory_manager;

// T_address is the type used for addressed.
// Must have operators to do difference (result ptrdiff_t) and ordering, and must not overflow in the specified range.
// The differenze is expressed "blocks". A block can allocate block_size bytes when mapped
// The smallest unit of allocation this buddy will allow is atom_size blocks.
// F_mmap(begin, end)  will yield an usable pointer to memory corresponding to a specific block
template<typename T_address, size_t block_size, size_t atom_size>
class buddymm {
    typedef T_address address_t;
    address_t range_begin;
    address_t range_end;
    int table_count;
    bitset_rope* tables;
    std::function<void*(address_t, size_t)> v_mmap;
    std::function<void(void*, size_t)> v_munmap;
    
    buddymm(address_t p_range_begin, address_t p_range_end):
        range_begin{p_range_begin},
        range_end{p_range_end},
        table_count{0}
    {}
    
    // initialize 
    void initialize_selfhosted(size_t buddy_structure_base_atom = 0) {
        size_t totalAtoms = (range_end - range_begin) / atom_size;

        size_t complete_atoms = 0;
        size_t partial_atoms = 0;
        char* last_allocation_address = nullptr;
        size_t last_allocation_usage = 0;

        // a temporary allocator which get space from base address and advances a pointer.
        // A bit of bootkeeping is necessary to map atoms and align structures.
        auto tmp_allocate = [&](size_t size){
            size_t atom_byte_size = block_size * atom_size;
            size_t alignment = 8;
            size = (size + alignment - 1) / alignment  * alignment;
            size_t remaining_bytes = partial_atoms * atom_byte_size;
            if (remaining_bytes >= size) {
                auto retAddr = last_allocation_address + last_allocation_usage;
                last_allocation_usage += size;
                return reinterpret_cast<void*>(retAddr);
            } else {
                size_t needed_atoms = (size + atom_byte_size - 1) / atom_byte_size;
                auto newalloc = v_mmap()
            }

            if (needed_atoms > allocated_atoms) {
                // we put the
                v_mmap()
            }
            

            void* address = 
        };
        
        while (totalAtoms > 0) {
            auto table_size = totalAtoms;

            totalAtoms >> 1;
            table_count++;
        }
        // stack allocation for temporary structures
        int table_sizes[table_count];
    }
    // Low level functions. Will return false and do nothing if the address is not in the expected state (i.e., opposite of requested)
    bool mark_used(address_t range_begin, address_t range_end);
    bool mark_unused(address_t range_begin, address_t range_end);

    address_t allocate(size_t atom_count)
    
}