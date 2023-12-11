#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>

#include <vulkan/vulkan.h>
#include <android/native_window_jni.h>

#include "GfxTypes.h"
#include "../Utils/ThreadSafeHandle.h"
#include "GfxTexture.h"
#include "GfxBuffer.h"

class TextureSampler;

struct DeviceConfig {
    VkInstance instance{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    uint32_t computeQueueFamilyIndex{};
    uint32_t computeQueueIndex{};
    std::string applicationName{};
    bool debugLayer{false};
};

struct DeviceStruct{
    VkInstance instance{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
};

struct BufferQueueStruct{
    VkQueue computeQueue{VK_NULL_HANDLE};
    VkQueue transferQueue{VK_NULL_HANDLE};
    VkQueue graphicsQueue{VK_NULL_HANDLE};

    uint32_t computeQueueFamilyIndex{};
    uint32_t transferQueueFamilyIndex{};
    uint32_t graphicsQueueFamilyIndex{};
};

class GfxDevice
{

public:
    GfxDevice(const DeviceConfig& config);
    ~GfxDevice();
    VkPhysicalDevice getPhysicalDevice(VkInstance instance);

    const VkDevice getDevice()const { return m_Device; }
    bool threadAssigned() { return true; }

    bool isInitialized() {return false; }

    void createSurface(ANativeWindow* window);
    void createSwapChain();
    void reCreateSwapchain();
private:
    void createSamplers();


private:
    VkInstance m_Instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    VkDevice m_Device{VK_NULL_HANDLE};

    VkQueue m_ComputeQueue{VK_NULL_HANDLE};
    uint32_t m_ComputeQueueFamilyIndex{};
    VkQueue m_transferQueue{VK_NULL_HANDLE};
    uint32_t m_TransferQueueFamilyIndex{};
    VkQueue m_GraphicsQueue{VK_NULL_HANDLE};
    uint32_t m_GraphicsQueueFamilyIndex{};

    VkPhysicalDeviceMemoryProperties m_MemoryProps{};
    VkSurfaceKHR m_Surface{VK_NULL_HANDLE};
    std::unordered_map<GfxSamplerType, std::unique_ptr<TextureSampler>> m_samplers;
    VkSurfaceTransformFlagBitsKHR m_PretransformFlag;
    VkSwapchainKHR m_SwapChain{VK_NULL_HANDLE};
    VkExtent2D m_DisplaySizeIdentity;
    VkExtent2D m_SwapchainExtent;
    std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainImageFormat;
    std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<GfxTexture>> m_textureMap;
    std::map<std::string, std::shared_ptr<GfxBuffer>> m_BufferMap;
};
using ThreadSafeGfxDevice = ThreadSafeHandle<GfxDevice>;