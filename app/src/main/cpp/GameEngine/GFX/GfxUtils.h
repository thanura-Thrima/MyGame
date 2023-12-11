#pragma once
#include <exception>
#include <string>
#include "../Utils/Definitions.h"
#include <vulkan/vulkan.h>

#define CHECK_VK(cmd)                                                                       \
    do {                                                                                    \
        VkResult res = (cmd);                                                               \
        if (res != VK_SUCCESS) {                                                            \
            LOGE(__FUNCTION__,"%s failed at %s:%d with %d", #cmd, __FILE__, __LINE__, res); \
            throw std::runtime_error("Vulkan failure : "+std::string(#cmd));                                                    \
        }                                                                                    \
    } while (0);


