#pragma once
#include <cstdint>

enum class GfxSamplerType : uint32_t {
    LINEAR_SAMPLER = 0,
    ANISO_SAMPLER,
    MIN_SAMPLER,
    LINEAR_CLAMP_SAMPLER,
    LINEAR_WRAP_SAMPLER,
    NEAREST_SAMPLER,
};
