#pragma once
#include <vulkan/vulkan.h>
#include "GfxTypes.h"
#include "../../Utils/Definitions.h"
#include "GfxDevice.h"

class TextureSampler{
public:
    NONCOPYABLE(TextureSampler);
    TextureSampler(const ThreadSafeGfxDevice& threadSafeDevice, GfxSamplerType type);
    ~TextureSampler(){};

    GfxSamplerType getType() const ;

    VkSampler getVkSampler() const;

private:
    VkSampler m_sampler;
    GfxSamplerType m_type;
    ThreadSafeGfxDevice m_device;
    static std::string m_TAG;
};
