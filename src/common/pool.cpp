#include <common/pool.h>
using namespace arawn::common;


Pool::flush_range::iterator::iterator(Pool* pool) : pool(pool) {
    ++*(this);
}
Pool::flush_range::iterator::value_type Pool::flush_range::iterator::operator*() const {
    return cached;
}

Pool::flush_range::iterator& Pool::flush_range::iterator::operator++() {
    if (!pool || pool->free.empty()) {
        pool = nullptr; 
        return *this;
    }
    
    uint32_t idx = pool->free.front();
    slot& slot = pool->slots[idx];
    
    if (slot.active != 0) {
        pool = nullptr; 
        return *this;
    }
    
    ++slot.version;
    cached = idx;
    pool->free.pop_front();
    
    return *this;
}

Pool::flush_range::iterator Pool::flush_range::iterator::operator++(int) {
    iterator tmp = *this;
    ++(*this);
    return tmp;   
}

bool arawn::common::operator==(const Pool::flush_range::iterator& lhs, Pool::flush_range::sentinel rhs) {
    return lhs.pool == nullptr;
}

Pool::flush_range::flush_range(Pool* pl) : pool(pl) { }

Pool::flush_range::iterator Pool::flush_range::begin() const {
    return iterator{ pool };
}

Pool::flush_range::sentinel Pool::flush_range::end() const {
    return { };
}

Handle<void> Pool::create() {
    uint32_t index;
    if (!free.empty() && !slots[free.back()].active) {
        index = free.back(); 
        free.pop_back();
    } else {
        index = static_cast<uint32_t>(slots.size());
        slots.emplace_back();
    }
    slot& slot = slots[index];
    
    slot.active = (1u << ARAWN_MAX_FRAMES) - 1u; // mark active for all frames
    return { index, slot.version };
}

void Pool::destroy(Handle<void> hnd) {
    free.push_front(hnd.index);
}

Pool::flush_range Pool::flush(uint32_t index) {
    uint8_t mask = ~(1 << index);
    for (uint32_t i = 0; i < free.size(); ++i) {
        slots[free[i]].active &= mask;
    }
    return flush_range{ this };
}

Pool::flush_range Pool::clear() {
    uint8_t mask = 0;
    for (uint32_t i = 0; i < free.size(); ++i) {
        slots[free[i]].active &= mask;
    }
    return flush_range{ this };
}

bool Pool::alive(Handle<void> hnd) const {
    return hnd.index < slots.size() && slots[hnd.index].version == hnd.version;
}
