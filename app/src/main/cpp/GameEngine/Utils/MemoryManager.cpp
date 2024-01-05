#include "MemoryManager.h"


std::shared_ptr<MemoryManager> MemoryManager::getMemoryManger() {
    if(m_MemoryManager == nullptr)
    {
        m_MemoryManager = std::shared_ptr<MemoryManager>(new MemoryManager());
    }
    return m_MemoryManager;
}

MemoryManager::MemoryManager(){}
MemoryManager::~MemoryManager(){}

void* MemoryManager::allocate(uint64 size, MEMORY_TAG tag) {

    return nullptr;
}

void MemoryManager::free(void* memBlock, uint64 size, MemoryManager::MEMORY_TAG tag) {
    throw std::runtime_error("wrong deletion");
    //delete memBlock;
}

void* MemoryManager::copy(void* dest, const void *source, uint64 size) {
    return nullptr;
}

void *MemoryManager::initialize(void* memBlock, uint64 size) {
    return nullptr;
}