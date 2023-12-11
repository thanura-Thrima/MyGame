#include "TextureSampler.h"
//#include "GfxTypes.h"
#include "GfxUtils.h"

VkSamplerCreateInfo getLinearConfig()
{
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Note: Default settings selected to correspond DX-backend.
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 1;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    return samplerCreateInfo;
}

TextureSampler::TextureSampler(const ThreadSafeGfxDevice& threadSafeDevice, GfxSamplerType type): m_device(threadSafeDevice) {
    VkSamplerCreateInfo samplerCreateInfo = getLinearConfig();
    switch (type) {
        case GfxSamplerType::LINEAR_SAMPLER: {
            // No need to modify anything
        } break;
        case GfxSamplerType::NEAREST_SAMPLER: {
            samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        } break;
        case GfxSamplerType::LINEAR_CLAMP_SAMPLER: {
            samplerCreateInfo.addressModeU = samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } break;
        case GfxSamplerType::LINEAR_WRAP_SAMPLER: {
            samplerCreateInfo.addressModeU = samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } break;
        case GfxSamplerType::MIN_SAMPLER: {
            LOGE(__FUNCTION__ ,"Not implemented.");
        } break;
        case GfxSamplerType::ANISO_SAMPLER: {
            samplerCreateInfo.anisotropyEnable = VK_TRUE;
            samplerCreateInfo.maxAnisotropy = 16;
        } break;
        default: {
            LOGE(__FUNCTION__,"Unkown sampler type.");

            break;
        }
    }
    auto dev = m_device.lock();
    if(dev != nullptr) {
        CHECK_VK(vkCreateSampler(dev->getDevice(), &samplerCreateInfo, nullptr, &m_sampler));
    }

    //const std::string samplerLabel = "sensing_sampler_" + getSamplerName(type);
    //dev->setDebugLabel(samplerLabel, VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(m_sampler));
}