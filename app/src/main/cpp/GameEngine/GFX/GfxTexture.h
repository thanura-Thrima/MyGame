#pragma once
#include <string>
#include <cstdio>
#include <vulkan/vulkan.h>

class GfxTexture
{
public:
    GfxTexture() = delete;
    GfxTexture(std::string name, uint32_t width, uint32_t height);
    virtual ~GfxTexture();

private:
    std::string m_name{};

    VkImageView m_SRV{VK_NULL_HANDLE};
    VkImageView m_UAV{VK_NULL_HANDLE};

    VkImage m_image{VK_NULL_HANDLE};
};
