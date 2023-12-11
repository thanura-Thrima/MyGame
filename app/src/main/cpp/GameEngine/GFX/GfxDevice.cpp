#include <vector>
#include <array>

#include "GfxUtils.h"
#include "GfxDevice.h"
#include "TextureSampler.h"

GfxDevice::GfxDevice(const DeviceConfig &config) {

    uint32_t queueIndex{};
    if(config.device != VK_NULL_HANDLE )
    {
        LOGD(__PRETTY_FUNCTION__,"vulkan device already available");
        LOGD(__PRETTY_FUNCTION__,"Using external device");
        m_Instance = config.instance;
        m_PhysicalDevice = config.physicalDevice;
        m_Device = config.device;
        m_ComputeQueueFamilyIndex = config.computeQueueFamilyIndex;
    }else {
        LOGD(__PRETTY_FUNCTION__,"vulkan device creation");
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = config.applicationName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "GameEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> enabledExtensions;
        {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> availableInstanceExtensions;
            availableInstanceExtensions.resize(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());

            std::vector<const char*> desiredExtensions = {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                    "VK_KHR_android_surface"
            };
            if(config.debugLayer)
            {
                desiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            for (const auto& extName : desiredExtensions) {
                auto it = std::find_if(availableInstanceExtensions.begin(), availableInstanceExtensions.end(),
                                       [&extName](const VkExtensionProperties& extProps) -> bool { return std::strcmp(extProps.extensionName, extName) == 0; });
                if (it != availableInstanceExtensions.end()) {
                    enabledExtensions.push_back(extName);
                }
            }
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledExtensions.data();

            std::vector<const char*> enabledLayers;

            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
            createInfo.ppEnabledLayerNames = enabledLayers.empty() ? nullptr : enabledLayers.data();
            CHECK_VK(vkCreateInstance(&createInfo, nullptr, &m_Instance));

            m_PhysicalDevice = getPhysicalDevice(m_Instance);
            if (m_PhysicalDevice == VK_NULL_HANDLE) {
                LOGE(__FUNCTION__ ,"No suitable physical device found.");
                throw std::runtime_error("no suitable physical device");
            }
        }
        {
            uint32_t count{};
            vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, nullptr);

            std::vector<VkQueueFamilyProperties> queueProps;
            queueProps.resize(count);
            vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, queueProps.data());

            bool computeQueueFound = false;
            bool graphicsQueueFound = false;
            for (uint32_t i = 0; i < count; i++) {
                if (!computeQueueFound && queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    m_ComputeQueueFamilyIndex = i;
                    computeQueueFound = true;
                }
                if (!graphicsQueueFound && queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    m_GraphicsQueueFamilyIndex = i;
                    graphicsQueueFound = true;
                }
            }
            if (!computeQueueFound || !graphicsQueueFound) {
                LOGE(__FUNCTION__ ,"Compute queue %d found, graphics queue %d found.", computeQueueFound , graphicsQueueFound );
                throw std::runtime_error("no suitable queue families");
            }
        }
        // Create device and command buffer pool.
        {
            std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
            std::array<float, 2> queuePriorities = {1.0, 1.0};

            VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
            deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            deviceQueueCreateInfo.pNext = nullptr;
            deviceQueueCreateInfo.queueCount = static_cast<int32_t>(queuePriorities.size());
            deviceQueueCreateInfo.pQueuePriorities = queuePriorities.data();
            deviceQueueCreateInfo.queueFamilyIndex = m_ComputeQueueFamilyIndex;
            deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

            // Presumably graphics and compute queue family index equals, but not necessarily.
            // In case not, separate device queue create info is needed with unique index.
            if (m_GraphicsQueueFamilyIndex != m_ComputeQueueFamilyIndex) {
                VkDeviceQueueCreateInfo deviceQueueCreateInfoGraphics = deviceQueueCreateInfo;
                deviceQueueCreateInfoGraphics.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
                deviceQueueCreateInfos.push_back(deviceQueueCreateInfoGraphics);
            }

            VkPhysicalDeviceFeatures features{};
            features.samplerAnisotropy = VK_TRUE;
            // Needed for shaders, which use plain Texture2D in hlsl without explicit image format.
            features.shaderStorageImageReadWithoutFormat = true;

            VkDeviceCreateInfo deviceInfo = {};
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
            deviceInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
            deviceInfo.pNext = nullptr;
            deviceInfo.pEnabledFeatures = &features;
            deviceInfo.enabledExtensionCount = 0;

            std::vector<const char*> enabledExtensions;
            {
                uint32_t extensionCount = 0;
                vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableDeviceExtensions;
                availableDeviceExtensions.resize(extensionCount);
                vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

                // List of extensions that we would like to enable if they are available.
                std::vector<const char*> desiredExtensions = {
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME
                };

                std::vector<const char*> enabledExtensions;

                for (const auto& extName : desiredExtensions) {
                    auto it = std::find_if(availableDeviceExtensions.begin(), availableDeviceExtensions.end(),
                                           [&extName](const VkExtensionProperties& extProps) -> bool { return std::strcmp(extProps.extensionName, extName) == 0; });
                    if (it != availableDeviceExtensions.end()) {
                        enabledExtensions.push_back(extName);
                    }
                }

                deviceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
                deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();
            }

            CHECK_VK(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_Device));
        }
    }
    vkGetDeviceQueue(m_Device, m_ComputeQueueFamilyIndex, queueIndex, &m_ComputeQueue);
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, queueIndex, &m_GraphicsQueue);

    // Read memory info to be able to alloc the resources later.
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProps);

/*    if (config.device == VK_NULL_HANDLE) {
        setDebugLabel("physical_device", VK_OBJECT_TYPE_PHYSICAL_DEVICE, reinterpret_cast<uint64_t>(getVkPhysicalDevice()));
        setDebugLabel("device", VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(getVkDevice()));
        setDebugLabel("compute_queue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(getVkQueueCompute()));
    }*/

    createSamplers();

//    auto allShaders = ComputeShader::getAllShaderNames();
//
//    for (const auto& name : allShaders) {
//        if (!getComputeShader(name)) {
//            LOGE("Shader load failed: %s", name);
//        }
//    }

    //ThreadSafeVulkanDevice dev(m_thisPtr);
    //m_dependencyManager = std::make_unique<VulkanDependencyTracker>();
    //m_resourceManager = std::make_unique<VulkanResourceManager>(dev);
    //m_commandBufferManager = std::make_unique<CommandBufferManager>(dev);
}

VkPhysicalDevice GfxDevice::getPhysicalDevice(VkInstance instance)
{
    uint32_t deviceCount = 0;
    CHECK_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    if (deviceCount == 0) {
        LOGE(__FUNCTION__,"No physical devices found.");
        return VK_NULL_HANDLE;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    CHECK_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
    // add code if more than one device in the system and need to get most matching physical Device
    return devices[0];
}

void GfxDevice::createSamplers() {
    //m_samplers.insert()

}

void GfxDevice::createSurface(ANativeWindow* window) {
    VkAndroidSurfaceCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.window = window;

    CHECK_VK(vkCreateAndroidSurfaceKHR(m_Instance, &create_info, nullptr, &m_Surface));
}


void GfxDevice::reCreateSwapchain() {

}

void GfxDevice::createSwapChain() {
    VkSurfaceCapabilitiesKHR  capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);

    uint32_t formatCount;
    std::vector<VkSurfaceFormatKHR> formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    std::vector<VkPresentModeKHR> presentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR( m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());
    }
    auto chooseSwapSurfaceFormat =
            [](const std::vector<VkSurfaceFormatKHR> &availableFormats) {
                for (const auto &availableFormat : availableFormats) {
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                    }
                }
                return availableFormats[0];
            };

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);

    // Please check
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
    // for a discourse on different present modes.
    //
    // VK_PRESENT_MODE_FIFO_KHR = Hard Vsync
    // This is always supported on Android phones
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    m_PretransformFlag = capabilities.currentTransform;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_DisplaySizeIdentity;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = m_PretransformFlag;

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    CHECK_VK(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));

    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
    m_SwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount,
                            m_SwapchainImages.data());

    m_SwapchainImageFormat = surfaceFormat.format;
    m_SwapchainExtent = m_DisplaySizeIdentity;
}
