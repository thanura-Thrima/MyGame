#pragma once
#include <string>
#include <cstdio>

#include <vulkan/vulkan.h>

class GfxBuffer{
public:
    GfxBuffer() = delete;
    GfxBuffer(std::string name, uint32_t size);
    virtual ~GfxBuffer();

private:
    std::string m_name;
};