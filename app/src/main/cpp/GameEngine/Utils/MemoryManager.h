#pragma once
#include <string>
#include <map>
#include <memory>

#include "Definitions.h"

class MemoryManager {
public:
    enum class MEMORY_TAG: int {
        MEMORY_TAG_UNKNOWN,

        MEMORY_TAG_COUNT
    } ;
    static std::shared_ptr<MemoryManager> getMemoryManger();
    void* allocate(uint64 size, MEMORY_TAG tag);
    void free(void* memBlock,uint64 size, MEMORY_TAG tag);
    void* initialize(void* memBlock,uint64 size);
    void* copy(void* dest, const void* source,uint64 size);
    void* setMemory(void* dest, uint8 value, uint64 size);
    std::string getMemoryUsage();
    virtual ~MemoryManager();

private:
    MemoryManager();

private:
    static std::shared_ptr<MemoryManager> m_MemoryManager;
    std::map<MEMORY_TAG,uint64> m_memoryTypeAllocation;
};

