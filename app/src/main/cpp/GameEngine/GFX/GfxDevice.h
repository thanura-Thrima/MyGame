#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <android/native_window_jni.h>

#include "GfxTypes.h"
#include "../Utils/ThreadSafeHandle.h"
#include "GfxTexture.h"
#include "GfxBuffer.h"

#include "../EntityComponent/Mesh.h"
class TextureSampler;

struct DeviceConfig {
    VkInstance instance{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    uint32_t computeQueueFamilyIndex{};
    uint32_t graphicsQueueFamilyIndex{};
    uint32_t transferQueueFamilyIndex{};
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

struct SwapchainImage {
    VkImage image;
    VkImageView imageView;
};

class GfxDevice
{

public:
    GfxDevice(const DeviceConfig& config);
    ~GfxDevice();
    VkPhysicalDevice getPhysicalDevice(VkInstance instance);

    const VkDevice getDevice()const { return m_DeviceStruct.device; }
    bool threadAssigned() { return true; }

    bool isInitialized() {return false; }

    void createSurface(ANativeWindow* window);
    void createSwapChain();
    void reCreateSwapchain();
private:
    void createSamplers();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createPushConstantRange();
    void createGraphicsPipeline();
    void createCommandPool();
    void createCommandBuffers();
    void createTextureSampler();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    void updateUniformBuffers(uint32_t imageIndex);
    void recordCommands(uint32_t currentImage);

    VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags,
                        VkMemoryPropertyFlags propFlags, VkDeviceMemory * imageMemory);

    static std::vector<char> readFile(const std::string &filename);
    static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);
    static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
                                 VkMemoryPropertyFlags bufferProperties, VkBuffer * buffer, VkDeviceMemory * bufferMemory);

private:
        // consolidated vulkan info
    DeviceStruct m_DeviceStruct{};
    BufferQueueStruct m_BufferQueueStruct{};

    // - Pools
    VkCommandPool m_GraphicsCommandPool;

    VkPhysicalDeviceMemoryProperties m_MemoryProps{};
    VkSurfaceKHR m_Surface{VK_NULL_HANDLE};
    std::unordered_map<GfxSamplerType, std::unique_ptr<TextureSampler>> m_samplers;
    VkSurfaceTransformFlagBitsKHR m_PretransformFlag;
    VkSwapchainKHR m_SwapChain{VK_NULL_HANDLE};
    VkRenderPass m_RenderPass {VK_NULL_HANDLE};
    VkPipeline m_GraphicsPipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
    VkExtent2D m_DisplaySizeIdentity;
    VkExtent2D m_SwapchainExtent;
    std::vector<SwapchainImage> m_SwapchainImages;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkFormat m_SwapchainImageFormat;
    std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<GfxTexture>> m_textureMap;
    std::map<std::string, std::shared_ptr<GfxBuffer>> m_BufferMap;

    VkImage m_DepthBufferImage;
    VkDeviceMemory m_DepthBufferImageMemory;
    VkImageView m_DepthBufferImageView;
    VkSampler m_TextureSampler;

    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkDescriptorSetLayout m_SamplerSetLayout;
    VkPushConstantRange m_pushConstantRange;
    VkDescriptorPool m_DescriptorPool;
    VkDescriptorPool m_SamplerDescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<VkDescriptorSet> m_SamplerDescriptorSets;

    std::vector<VkBuffer> vpUniformBuffer;
    std::vector<VkDeviceMemory> vpUniformBufferMemory;

    std::vector<VkBuffer> modelDUniformBuffer;
    std::vector<VkDeviceMemory> modelDUniformBufferMemory;

    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection;

    int currentFrame = 0;

    // Scene Objects
    std::vector<Mesh> meshList;
};
using ThreadSafeGfxDevice = ThreadSafeHandle<GfxDevice>;