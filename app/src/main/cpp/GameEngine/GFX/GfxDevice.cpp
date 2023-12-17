#include <vector>
#include <array>
#include <fstream>

#include "GfxUtils.h"
#include "GfxDevice.h"
#include "TextureSampler.h"

GfxDevice::GfxDevice(const DeviceConfig &config) {

    uint32_t queueIndex{};
    if(config.device != VK_NULL_HANDLE )
    {
        LOGD(__PRETTY_FUNCTION__,"vulkan device already available");
        LOGD(__PRETTY_FUNCTION__,"Using external device");
            // replacing with new consolidated struct
        m_DeviceStruct.instance = config.instance;
        m_DeviceStruct.physicalDevice = config.physicalDevice;
        m_DeviceStruct.device = config.device;

        m_BufferQueueStruct.computeQueueFamilyIndex = config.computeQueueFamilyIndex;
        m_BufferQueueStruct.graphicsQueueFamilyIndex = config.graphicsQueueFamilyIndex;
        m_BufferQueueStruct.transferQueueFamilyIndex = config.transferQueueFamilyIndex;

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


        {
            std::vector<const char*> enabledExtensions;
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> availableInstanceExtensions;
            availableInstanceExtensions.resize(extensionCount);
            //TODO need VKResult check?
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

            //TODO add debug layer
            std::vector<const char*> enabledLayers;

            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
            createInfo.ppEnabledLayerNames = enabledLayers.empty() ? nullptr : enabledLayers.data();
            CHECK_VK(vkCreateInstance(&createInfo, nullptr, &m_DeviceStruct.instance));

            m_DeviceStruct.physicalDevice = getPhysicalDevice(m_DeviceStruct.instance);
            if (m_DeviceStruct.physicalDevice == VK_NULL_HANDLE) {
                LOGE(__FUNCTION__ ,"No suitable physical device found.");
                throw std::runtime_error("no suitable physical device");
            }
        }
        {
            uint32_t count{};
            vkGetPhysicalDeviceQueueFamilyProperties(m_DeviceStruct.physicalDevice, &count, nullptr);
            std::vector<VkQueueFamilyProperties> queueProps;
            queueProps.resize(count);
            vkGetPhysicalDeviceQueueFamilyProperties(m_DeviceStruct.physicalDevice, &count, queueProps.data());

            bool computeQueueFound = false;
            bool graphicsQueueFound = false;
            bool transferQueueFound = false;
            for (uint32_t i = 0; i < count; i++) {
                if (!computeQueueFound && queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    m_BufferQueueStruct.computeQueueFamilyIndex = i;
                    computeQueueFound = true;
                }
                if (!transferQueueFound && queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    m_BufferQueueStruct.transferQueueFamilyIndex = i;
                    transferQueueFound = true;
                }
                if( !graphicsQueueFound && queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    m_BufferQueueStruct.graphicsQueueFamilyIndex = i;
                    graphicsQueueFound = true;
                }
            }
            if (!computeQueueFound || !graphicsQueueFound || !transferQueueFound) {
                LOGE(__FUNCTION__ ,"Compute queue %d found, graphics queue %d found., transfer queue %d found ", computeQueueFound , graphicsQueueFound, transferQueueFound );
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
            deviceQueueCreateInfo.queueFamilyIndex = m_BufferQueueStruct.computeQueueFamilyIndex;
            deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

            // Presumably graphics and compute queue family index equals, but not necessarily.
            // In case not, separate device queue create info is needed with unique index.
            if (m_BufferQueueStruct.graphicsQueueFamilyIndex != m_BufferQueueStruct.computeQueueFamilyIndex) {
                VkDeviceQueueCreateInfo deviceQueueCreateInfoGraphics = deviceQueueCreateInfo;
                deviceQueueCreateInfoGraphics.queueFamilyIndex = m_BufferQueueStruct.graphicsQueueFamilyIndex;
                deviceQueueCreateInfos.push_back(deviceQueueCreateInfoGraphics);
            }

            if(m_BufferQueueStruct.graphicsQueueFamilyIndex != m_BufferQueueStruct.transferQueueFamilyIndex)
            {
                VkDeviceQueueCreateInfo deviceQueueCreateInfoTransfer = deviceQueueCreateInfo;
                deviceQueueCreateInfoTransfer.queueFamilyIndex = m_BufferQueueStruct.transferQueueFamilyIndex;
                deviceQueueCreateInfos.push_back(deviceQueueCreateInfoTransfer);
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

            {
                uint32_t extensionCount = 0;
                CHECK_VK(vkEnumerateDeviceExtensionProperties(m_DeviceStruct.physicalDevice, nullptr, &extensionCount, nullptr));

                std::vector<VkExtensionProperties> availableDeviceExtensions;
                availableDeviceExtensions.resize(extensionCount);
                CHECK_VK(vkEnumerateDeviceExtensionProperties(m_DeviceStruct.physicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data()));

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

            CHECK_VK(vkCreateDevice(m_DeviceStruct.physicalDevice, &deviceInfo, nullptr, &m_DeviceStruct.device));
        }
    }
    vkGetDeviceQueue(m_DeviceStruct.device, m_BufferQueueStruct.computeQueueFamilyIndex, queueIndex, &m_BufferQueueStruct.computeQueue);
    vkGetDeviceQueue(m_DeviceStruct.device, m_BufferQueueStruct.graphicsQueueFamilyIndex, queueIndex, &m_BufferQueueStruct.graphicsQueue);

    // Read memory info to be able to alloc the resources later.
    vkGetPhysicalDeviceMemoryProperties(m_DeviceStruct.physicalDevice, &m_MemoryProps);

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

    CHECK_VK(vkCreateAndroidSurfaceKHR(m_DeviceStruct.instance, &create_info, nullptr, &m_Surface));
}


void GfxDevice::reCreateSwapchain() {

}

void GfxDevice::createSwapChain() {
    VkSurfaceCapabilitiesKHR capabilities;
    CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_DeviceStruct.physicalDevice, m_Surface,
                                                       &capabilities));

    uint32_t formatCount = 0;
    std::vector<VkSurfaceFormatKHR> formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_DeviceStruct.physicalDevice, m_Surface, &formatCount,
                                         nullptr);
    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_DeviceStruct.physicalDevice, m_Surface, &formatCount,
                                             formats.data());
    }

    uint32_t presentModeCount;
    std::vector<VkPresentModeKHR> presentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_DeviceStruct.physicalDevice, m_Surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        CHECK_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_DeviceStruct.physicalDevice, m_Surface,
                                                           &presentModeCount, presentModes.data()));
    }
    auto chooseSwapSurfaceFormat =
            [](const std::vector<VkSurfaceFormatKHR> &availableFormats) {
                for (const auto &availableFormat: availableFormats) {
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

    CHECK_VK(vkCreateSwapchainKHR(m_DeviceStruct.device, &createInfo, nullptr, &m_SwapChain));

    vkGetSwapchainImagesKHR(m_DeviceStruct.device, m_SwapChain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_DeviceStruct.device, m_SwapChain, &imageCount,
                            images.data());

    m_SwapchainImageFormat = surfaceFormat.format;
    m_SwapchainExtent = m_DisplaySizeIdentity;

    for (VkImage image: images) {
        // Store image handle
        SwapchainImage swapChainImage = {};
        swapChainImage.image = image;
        swapChainImage.imageView = createImageView(image, m_SwapchainImageFormat,
                                                   VK_IMAGE_ASPECT_COLOR_BIT);

        // Add to swapchain image list
        m_SwapchainImages.push_back(swapChainImage);
    }


    VkFormat depthFormat = chooseSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Create Depth Buffer Image
    m_DepthBufferImage = createImage(m_SwapchainExtent.width, m_SwapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_DepthBufferImageMemory);

    // Create Depth Buffer Image View
    m_DepthBufferImageView = createImageView(m_DepthBufferImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);


    m_SwapchainFramebuffers.resize(m_SwapchainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < m_SwapchainFramebuffers.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
                m_SwapchainImages[i].imageView,
                m_DepthBufferImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_RenderPass;										// Render Pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();							// List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = m_SwapchainExtent.width;								// Framebuffer width
        framebufferCreateInfo.height = m_SwapchainExtent.height;								// Framebuffer height
        framebufferCreateInfo.layers = 1;													// Framebuffer layers

        CHECK_VK(vkCreateFramebuffer(m_DeviceStruct.device, &framebufferCreateInfo, nullptr, &m_SwapchainFramebuffers[i]));
    }

}

VkImageView GfxDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;											// Image to create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;						// Type of image (1D, 2D, 3D, Cube, etc)
    viewCreateInfo.format = format;											// Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Allows remapping of rgba components to other rgba values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;				// Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;						// Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;							// Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;						// Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;							// Number of array levels to view

    // Create image view and return it
    VkImageView imageView;
    CHECK_VK(vkCreateImageView(m_DeviceStruct.device, &viewCreateInfo, nullptr, &imageView));

    return imageView;
}

void GfxDevice::createRenderPass()
{
    // ATTACHMENTS
    // Colour attachment of render pass
    VkAttachmentDescription colourAttachment = {};
    colourAttachment.format = m_SwapchainImageFormat;						// Format to use for attachment
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			// Describes what to do with attachment after rendering
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)


    // Depth attachment of render pass
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = chooseSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // REFERENCES
    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colourAttachmentReference = {};
    colourAttachmentReference.attachment = 0;
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type subpass is to be bound to
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)
    // But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;


    // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
    // But must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    std::array<VkAttachmentDescription, 2> renderPassAttachments = { colourAttachment, depthAttachment };

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    CHECK_VK(vkCreateRenderPass(m_DeviceStruct.device, &renderPassCreateInfo, nullptr, &m_RenderPass));
}
VkFormat GfxDevice::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    // Loop through options and find compatible one
    for (VkFormat format : formats)
    {
        // Get properties for give format on this device
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_DeviceStruct.physicalDevice, format, &properties);

        // Depending on tiling choice, need to check for different bit flag
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a matching format!");
}

void GfxDevice::createDescriptorSetLayout()
{
    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
    // UboViewProjection Binding Info
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
    vpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
    vpLayoutBinding.pImmutableSamplers = nullptr;							// For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

    // Model Binding Info
    /*VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 1;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr;*/

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

    // Create Descriptor Set Layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());	// Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();								// Array of binding infos

    // Create Descriptor Set Layout
    VkResult result = vkCreateDescriptorSetLayout(m_DeviceStruct.device, &layoutCreateInfo, nullptr, &m_DescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Set Layout!");
    }

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    // Create a Descriptor Set Layout with given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor Set Layout
    CHECK_VK(vkCreateDescriptorSetLayout(m_DeviceStruct.device, &textureLayoutCreateInfo, nullptr, &m_SamplerSetLayout));
}

void GfxDevice::createPushConstantRange()
{
    // Define push constant values (no 'create' needed!)
    m_pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	// Shader stage push constant will go to
    m_pushConstantRange.offset = 0;								// Offset into given data to pass to push constant
    m_pushConstantRange.size = sizeof(Model);						// Size of data being passed
}


void GfxDevice::createGraphicsPipeline()
{
    // Read in SPIR-V code of shaders
    auto vertexShaderCode = readFile("Shaders/vert.spv");
    auto fragmentShaderCode = readFile("Shaders/frag.spv");

    // Create Shader Modules
    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    // -- SHADER STAGE CREATION INFORMATION --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;				// Shader Stage name
    vertexShaderCreateInfo.module = vertexShaderModule;						// Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";									// Entry point in to shader

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;				// Shader Stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;						// Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";									// Entry point in to shader

    // Put shader stage creation info in to array
    // Graphics Pipeline creation info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // How the data for a single vertex (including info such as position, colour, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;									// Can bind multiple streams of data, this defines which one
    bindingDescription.stride = sizeof(Vertex);						// Size of a single vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
    // VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
    // VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

    // Position Attribute
    attributeDescriptions[0].binding = 0;							// Which binding the data is at (should be same as above)
    attributeDescriptions[0].location = 0;							// Location in shader where data will be read from
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
    attributeDescriptions[0].offset = offsetof(Vertex, position);		// Where this attribute is defined in the data for a single vertex

    // Colour Attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    // Texture Attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, tex);

    // -- VERTEX INPUT --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;											// List of Vertex Binding Descriptions (data spacing/stride information)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();								// List of Vertex Attribute Descriptions (data format and where to bind to/from)


    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		// Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;					// Allow overriding of "strip" topology to start new primitives


    // -- VIEWPORT & SCISSOR --
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;									// x start coordinate
    viewport.y = 0.0f;									// y start coordinate
    viewport.width = (float)m_SwapchainExtent.width;		// width of viewport
    viewport.height = (float)m_SwapchainExtent.height;	// height of viewport
    viewport.minDepth = 0.0f;							// min framebuffer depth
    viewport.maxDepth = 1.0f;							// max framebuffer depth

    // Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = { 0,0 };							// Offset to use region from
    scissor.extent = m_SwapchainExtent;					// Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;


    // -- DYNAMIC STATES --
    // Dynamic states to enable
    //std::vector<VkDynamicState> dynamicStateEnables;
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    //// Dynamic State creation info
    //VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    //dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();


    // -- RASTERIZER --
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;					// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;			// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;			// How to handle filling points between vertices
    rasterizerCreateInfo.lineWidth = 1.0f;								// How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;				// Which face of a tri to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;					// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)


    // -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment


    // -- BLENDING --
    // Blending decides how to blend a new colour being written to a fragment, with the old value

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colourState = {};
    colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colours to apply blending to
                                 | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colourState.blendEnable = VK_TRUE;													// Enable blending

    // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
    colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colourState.colorBlendOp = VK_BLEND_OP_ADD;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    //			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

    colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourState.alphaBlendOp = VK_BLEND_OP_ADD;
    // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
    colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
    colourBlendingCreateInfo.attachmentCount = 1;
    colourBlendingCreateInfo.pAttachments = &colourState;

    // -- PIPELINE LAYOUT --
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { m_DescriptorSetLayout, m_SamplerSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &m_pushConstantRange;

    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(m_DeviceStruct.device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Pipeline Layout!");
    }


    // -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;				// Enable checking depth to determine fragment write
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;				// Enable writing to depth buffer (to replace old values)
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;		// Comparison operation that allows an overwrite (is in front)
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth Bounds Test: Does the depth value exist between two bounds
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;			// Enable Stencil Test


    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;									// Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;							// List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;		// All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.layout = m_PipelineLayout;							// Pipeline Layout pipeline should use
    pipelineCreateInfo.renderPass = m_RenderPass;							// Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

    // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
    pipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

    // Create Graphics Pipeline
    CHECK_VK(vkCreateGraphicsPipelines(m_DeviceStruct.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_GraphicsPipeline));

    // Destroy Shader Modules, no longer needed after Pipeline created
    vkDestroyShaderModule(m_DeviceStruct.device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(m_DeviceStruct.device, vertexShaderModule, nullptr);
}

std::vector<char> GfxDevice::readFile(const std::string &filename)
{
    // Open stream from given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading from end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    // Check if file stream successfully opened
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open a file!");
    }

    // Get current read position and use to resize file buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> fileBuffer(fileSize);

    // Move read position (seek to) the start of the file
    file.seekg(0);

    // Read the file data into the buffer (stream "fileSize" in total)
    file.read(fileBuffer.data(), fileSize);

    // Close stream
    file.close();

    return fileBuffer;
}

VkShaderModule GfxDevice::createShaderModule(const std::vector<char>& code)
{
    // Shader Module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();										// Size of code
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());		// Pointer to code (of uint32_t pointer type)

    VkShaderModule shaderModule;
    CHECK_VK(vkCreateShaderModule(m_DeviceStruct.device, &shaderModuleCreateInfo, nullptr, &shaderModule));
    return shaderModule;
}

VkImage GfxDevice::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory * imageMemory)
{
    // CREATE IMAGE
    // Image Creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;						// Type of image (1D, 2D, or 3D)
    imageCreateInfo.extent.width = width;								// Width of image extent
    imageCreateInfo.extent.height = height;								// Height of image extent
    imageCreateInfo.extent.depth = 1;									// Depth of image (just 1, no 3D aspect)
    imageCreateInfo.mipLevels = 1;										// Number of mipmap levels
    imageCreateInfo.arrayLayers = 1;									// Number of levels in image array
    imageCreateInfo.format = format;									// Format type of image
    imageCreateInfo.tiling = tiling;									// How image data should be "tiled" (arranged for optimal reading)
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Layout of image data on creation
    imageCreateInfo.usage = useFlags;									// Bit flags defining what image will be used for
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples for multi-sampling
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Whether image can be shared between queues

    // Create image
    VkImage image;
    VkResult result = vkCreateImage(m_DeviceStruct.device, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image!");
    }

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(m_DeviceStruct.device, image, &memoryRequirements);

    // Allocate memory using image requirements and user defined properties
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(m_DeviceStruct.physicalDevice, memoryRequirements.memoryTypeBits, propFlags);

    result = vkAllocateMemory(m_DeviceStruct.device, &memoryAllocInfo, nullptr, imageMemory);


    // Connect memory to image
    vkBindImageMemory(m_DeviceStruct.device, image, *imageMemory, 0);

    return image;
}

uint32_t GfxDevice::findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((allowedTypes & (1 << i))														// Index of memory type must match corresponding bit in allowedTypes
            && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)	// Desired property bit flags are part of memory type's property flags
        {
            // This memory type is valid, so return its index
            return i;
        }
    }
    return 0;
}

void GfxDevice::createCommandPool()
{

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_BufferQueueStruct.graphicsQueueFamilyIndex;	// Queue Family type that buffers from this command pool will use

    // Create a Graphics Queue Family Command Pool
    CHECK_VK(vkCreateCommandPool(m_DeviceStruct.device, &poolInfo, nullptr, &m_GraphicsCommandPool));
}

void GfxDevice::createCommandBuffers()
{
    // Resize command buffer count to have one for each framebuffer
    m_CommandBuffers.resize(m_SwapchainFramebuffers.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = m_GraphicsCommandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    CHECK_VK(vkAllocateCommandBuffers(m_DeviceStruct.device, &cbAllocInfo, m_CommandBuffers.data()));
}

void GfxDevice::createTextureSampler()
{
    // Sampler Creation Info
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U (x) direction
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in V (y) direction
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in W (z) direction
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
    samplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
    samplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
    samplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
    samplerCreateInfo.anisotropyEnable = VK_TRUE;						// Enable Anisotropy
    samplerCreateInfo.maxAnisotropy = 16;								// Anisotropy sample level

    CHECK_VK(vkCreateSampler(m_DeviceStruct.device, &samplerCreateInfo, nullptr, &m_TextureSampler));
}

void GfxDevice::createUniformBuffers()
{
    // ViewProjection buffer size
    VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

    // Model buffer size
    //VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (and by extension, command buffer)
    vpUniformBuffer.resize(m_SwapchainImages.size());
    vpUniformBufferMemory.resize(m_SwapchainImages.size());
    //modelDUniformBuffer.resize(swapChainImages.size());
    //modelDUniformBufferMemory.resize(swapChainImages.size());

    // Create Uniform buffers
    for (size_t i = 0; i < m_SwapchainImages.size(); i++)
    {
        createBuffer(m_DeviceStruct.physicalDevice, m_DeviceStruct.device, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

        /*createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);*/
    }
}

void GfxDevice::createDescriptorSets()
{
    // Resize Descriptor Set list so one for every buffer
    m_DescriptorSets.resize(m_SwapchainImages.size());

    std::vector<VkDescriptorSetLayout> setLayouts(m_SwapchainImages.size(), m_DescriptorSetLayout);

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_DescriptorPool;									// Pool to allocate Descriptor Set from
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapchainImages.size());// Number of sets to allocate
    setAllocInfo.pSetLayouts = setLayouts.data();									// Layouts to use to allocate sets (1:1 relationship)

    // Allocate descriptor sets (multiple)
    VkResult result = vkAllocateDescriptorSets(m_DeviceStruct.device, &setAllocInfo, m_DescriptorSets.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Descriptor Sets!");
    }

    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < m_SwapchainImages.size(); i++)
    {
        // VIEW PROJECTION DESCRIPTOR
        // Buffer info and data offset info
        VkDescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = vpUniformBuffer[i];		// Buffer to get data from
        vpBufferInfo.offset = 0;						// Position of start of data
        vpBufferInfo.range = sizeof(UboViewProjection);				// Size of data

        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vpSetWrite.dstSet = m_DescriptorSets[i];								// Descriptor Set to update
        vpSetWrite.dstBinding = 0;											// Binding to update (matches with binding on layout/shader)
        vpSetWrite.dstArrayElement = 0;									// Index in array to update
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Type of descriptor
        vpSetWrite.descriptorCount = 1;									// Amount to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;							// Information about buffer data to bind

        // MODEL DESCRIPTOR
        // Model Buffer Binding Info
        /*VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = modelDUniformBuffer[i];
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = modelUniformAlignment;

        VkWriteDescriptorSet modelSetWrite = {};
        modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelSetWrite.dstSet = descriptorSets[i];
        modelSetWrite.dstBinding = 1;
        modelSetWrite.dstArrayElement = 0;
        modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        modelSetWrite.descriptorCount = 1;
        modelSetWrite.pBufferInfo = &modelBufferInfo;*/

        // List of Descriptor Set Writes
        std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(m_DeviceStruct.device, static_cast<uint32_t>(setWrites.size()), setWrites.data(),
                               0, nullptr);
    }
}

void GfxDevice::updateUniformBuffers(uint32_t imageIndex)
{
    // Copy VP data
    void * data;
    vkMapMemory(m_DeviceStruct.device, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
    memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
    vkUnmapMemory(m_DeviceStruct.device, vpUniformBufferMemory[imageIndex]);

    // Copy Model data
    /*for (size_t i = 0; i < meshList.size(); i++)
    {
        UboModel * thisModel = (UboModel *)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
        *thisModel = meshList[i].getModel();
    }

    // Map the list of model data
    vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
    memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
    vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);*/
}

void GfxDevice::recordCommands(uint32_t currentImage)
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Information about how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_RenderPass;							// Render Pass to begin
    renderPassBeginInfo.renderArea.offset = { 0, 0 };						// Start point of render pass in pixels
    renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;				// Size of region to run render pass on (starting at offset)

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f };
    clearValues[1].depthStencil.depth = 1.0f;

    renderPassBeginInfo.pClearValues = clearValues.data();					// List of clear values
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

    renderPassBeginInfo.framebuffer = m_SwapchainFramebuffers[currentImage];

    // Start recording commands to command buffer!
    VkResult result = vkBeginCommandBuffer(m_CommandBuffers[currentImage], &bufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to start recording a Command Buffer!");
    }
    // Begin Render Pass
    vkCmdBeginRenderPass(m_CommandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind Pipeline to be used in render pass
    vkCmdBindPipeline(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,m_GraphicsPipeline);

    for (size_t j = 0; j < meshList.size(); j++)
    {
        VkBuffer vertexBuffers[] = { meshList[j].getVertexBuffer() };					// Buffers to bind
        VkDeviceSize offsets[] = { 0 };												// Offsets into buffers being bound
        vkCmdBindVertexBuffers(m_CommandBuffers[currentImage], 0, 1, vertexBuffers, offsets);	// Command to bind vertex buffer before drawing with them

        // Bind mesh index buffer, with 0 offset and using the uint32 type
        vkCmdBindIndexBuffer(m_CommandBuffers[currentImage], meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Dynamic Offset Amount
        // uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;
        // "Push" constants to given shader stage directly (no buffer)
        vkCmdPushConstants(
                m_CommandBuffers[currentImage],
                m_PipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,		// Stage to push constants to
                0,								// Offset of push constants to update
                sizeof(Model),					// Size of data being pushed
                meshList[j].getModel());		// Actual data being pushed (can be array)

        std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_DescriptorSets[currentImage],
                                                              m_SamplerDescriptorSets[meshList[j].getTexId()] };
        // Bind Descriptor Sets
        vkCmdBindDescriptorSets(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                                0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);
        // Execute pipeline
        vkCmdDrawIndexed(m_CommandBuffers[currentImage], meshList[j].getIndexCount(), 1, 0, 0, 0);
    }
    // End Render Pass
    vkCmdEndRenderPass(m_CommandBuffers[currentImage]);

    // Stop recording to command buffer
    CHECK_VK(vkEndCommandBuffer(m_CommandBuffers[currentImage]));
}

void GfxDevice::createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
                         VkMemoryPropertyFlags bufferProperties, VkBuffer * buffer, VkDeviceMemory * bufferMemory)
{
    // CREATE VERTEX BUFFER
    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;								// Size of buffer (size of 1 vertex * number of vertices)
    bufferInfo.usage = bufferUsage;								// Multiple types of buffer possible
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Similar to Swap Chain images, can share vertex buffers

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vertex Buffer!");
    }

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,		// Index of memory type on Physical Device that has required bit flags
                                                          bufferProperties);																						// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT	: CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
    }

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}