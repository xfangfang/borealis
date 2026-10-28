/*
    Copyright 2026 xfangfang / ps5-wiliwili contributors

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/platforms/driver/vulkan.hpp>
#include <borealis/core/logger.hpp>

#if defined(BOREALIS_USE_VULKAN)

#ifdef __SDL2__
#include <SDL2/SDL_vulkan.h>
#endif

#include <algorithm>
#include <cstring>
#include <set>

#if defined(PS5)
extern "C" int sceSystemServiceHideSplashScreen(void);
#endif

namespace brls
{

std::unique_ptr<VulkanContext> VULKAN_CONTEXT;

#ifdef __SDL2__
VulkanContext::VulkanContext(SDL_Window* window, int width, int height)
    : window(window), windowWidth(width), windowHeight(height)
{
    if (!this->initVulkan(width, height))
    {
        Logger::error("vulkan: failed to initialize VulkanContext");
    }
}
#endif

VulkanContext::~VulkanContext()
{
    this->unInitVulkan();
}

double VulkanContext::getScaleFactor()
{
    return this->scaleFactor;
}

void VulkanContext::clear(NVGcolor color)
{
    this->clearColor = color;
}

void VulkanContext::setSwapInterval(int value)
{
    this->swapInterval = value;
}

VKNVGCreateInfo VulkanContext::getNVGCreateInfo()
{
    VKNVGCreateInfo info{};
    info.gpu                 = this->physicalDevice;
    info.device              = this->device;
    info.renderpass          = this->renderPass;
    info.cmdBuffer           = this->commandBuffers.data();
    info.commandPool         = this->commandPool;
    info.swapchainImageCount = static_cast<uint32_t>(this->swapchainImages.size());
    info.currentFrame        = &this->currentFrame;
    info.allocator           = nullptr;
    return info;
}

bool VulkanContext::initVulkan(int width, int height)
{
    Logger::info("vulkan: initializing Vulkan subsystem");
    if (!createInstance()) return false;
    if (!createSurface()) return false;
    if (!selectPhysicalDevice()) return false;
    if (!createLogicalDevice()) return false;
    if (!createSwapchain(width, height)) return false;
    if (!createImageViews()) return false;
    if (!createRenderPass()) return false;
    if (!createDepthResources()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPool()) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;

    Logger::info("vulkan: initialized successfully ({}x{}, {} images)", 
                 this->swapchainExtent.width, this->swapchainExtent.height, this->swapchainImages.size());
    return true;
}

void VulkanContext::unInitVulkan()
{
    if (this->device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(this->device);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (i < (int)this->renderFinishedSemaphores.size() && this->renderFinishedSemaphores[i])
                vkDestroySemaphore(this->device, this->renderFinishedSemaphores[i], nullptr);
            if (i < (int)this->imageAvailableSemaphores.size() && this->imageAvailableSemaphores[i])
                vkDestroySemaphore(this->device, this->imageAvailableSemaphores[i], nullptr);
            if (i < (int)this->inFlightFences.size() && this->inFlightFences[i])
                vkDestroyFence(this->device, this->inFlightFences[i], nullptr);
        }

        cleanupSwapchain();

        if (this->commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(this->device, this->commandPool, nullptr);
            this->commandPool = VK_NULL_HANDLE;
        }

        if (this->renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(this->device, this->renderPass, nullptr);
            this->renderPass = VK_NULL_HANDLE;
        }

        vkDestroyDevice(this->device, nullptr);
        this->device = VK_NULL_HANDLE;
    }

    if (this->surface != VK_NULL_HANDLE && this->instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        this->surface = VK_NULL_HANDLE;
    }

    if (this->instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(this->instance, nullptr);
        this->instance = VK_NULL_HANDLE;
    }
}

bool VulkanContext::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "wiliwili";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 6, 0);
    appInfo.pEngineName        = "borealis";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    std::vector<const char*> extensions;

#if !defined(PS5)
#ifdef __SDL2__
    if (this->window)
    {
        unsigned int count = 0;
        if (SDL_Vulkan_GetInstanceExtensions(this->window, &count, nullptr))
        {
            extensions.resize(count);
            SDL_Vulkan_GetInstanceExtensions(this->window, &count, extensions.data());
        }
    }
#endif
#endif

    // Ensure surface and display extensions are present
    auto addExtensionIfMissing = [&extensions](const char* ext) {
        for (const char* e : extensions)
        {
            if (strcmp(e, ext) == 0) return;
        }
        extensions.push_back(ext);
    };

    addExtensionIfMissing(VK_KHR_SURFACE_EXTENSION_NAME);
    addExtensionIfMissing(VK_KHR_DISPLAY_EXTENSION_NAME);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult res = vkCreateInstance(&createInfo, nullptr, &this->instance);
    if (res != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to create Vulkan instance (error {})", (int)res);
        return false;
    }
    return true;
}

bool VulkanContext::createSurface()
{
#if !defined(PS5)
#ifdef __SDL2__
    if (this->window && SDL_Vulkan_CreateSurface(this->window, this->instance, &this->surface))
    {
        Logger::info("vulkan: created surface via SDL_Vulkan_CreateSurface");
        return true;
    }
#endif
#endif

    // Fallback: VK_KHR_display (PS5 Native VideoOut)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
    if (deviceCount == 0) return false;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());
    VkPhysicalDevice targetGpu = devices[0];

    uint32_t displayCount = 0;
    vkGetPhysicalDeviceDisplayPropertiesKHR(targetGpu, &displayCount, nullptr);
    if (displayCount == 0)
    {
        Logger::error("vulkan: no displays found for VK_KHR_display");
        return false;
    }

    std::vector<VkDisplayPropertiesKHR> displays(displayCount);
    vkGetPhysicalDeviceDisplayPropertiesKHR(targetGpu, &displayCount, displays.data());

    uint32_t modeCount = 0;
    vkGetDisplayModePropertiesKHR(targetGpu, displays[0].display, &modeCount, nullptr);
    if (modeCount == 0)
    {
        Logger::error("vulkan: no display modes found");
        return false;
    }

    std::vector<VkDisplayModePropertiesKHR> modes(modeCount);
    vkGetDisplayModePropertiesKHR(targetGpu, displays[0].display, &modeCount, modes.data());

    VkDisplaySurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType           = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.displayMode     = modes[0].displayMode;
    surfaceCreateInfo.planeIndex      = 0;
    surfaceCreateInfo.planeStackIndex = 0;
    surfaceCreateInfo.transform       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    surfaceCreateInfo.globalAlpha     = 1.0f;
    surfaceCreateInfo.alphaMode       = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
    surfaceCreateInfo.imageExtent     = modes[0].parameters.visibleRegion;

    VkResult res = vkCreateDisplayPlaneSurfaceKHR(this->instance, &surfaceCreateInfo, nullptr, &this->surface);
    if (res != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to create display surface ({})", (int)res);
        return false;
    }

    Logger::info("vulkan: created surface via VK_KHR_display ({}x{})", 
                 surfaceCreateInfo.imageExtent.width, surfaceCreateInfo.imageExtent.height);

#if defined(PS5)
    sceSystemServiceHideSplashScreen();
#endif

    return true;
}

bool VulkanContext::selectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
    if (deviceCount == 0) return false;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

    for (const auto& deviceCandidate : devices)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(deviceCandidate, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(deviceCandidate, &queueFamilyCount, queueFamilies.data());

        int graphicsIdx = -1;
        int presentIdx  = -1;

        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                graphicsIdx = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(deviceCandidate, i, this->surface, &presentSupport);
            if (presentSupport)
                presentIdx = i;

            if (graphicsIdx >= 0 && presentIdx >= 0)
                break;
        }

        if (graphicsIdx >= 0 && presentIdx >= 0)
        {
            this->physicalDevice             = deviceCandidate;
            this->graphicsQueueFamilyIndex   = static_cast<uint32_t>(graphicsIdx);
            this->presentQueueFamilyIndex    = static_cast<uint32_t>(presentIdx);

            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(deviceCandidate, &props);
            Logger::info("vulkan: selected physical device: {}", props.deviceName);
            return true;
        }
    }

    Logger::error("vulkan: no suitable physical device found");
    return false;
}

bool VulkanContext::createLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {this->graphicsQueueFamilyIndex, this->presentQueueFamilyIndex};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_FALSE;

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult res = vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device);
    if (res != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to create logical device ({})", (int)res);
        return false;
    }

    vkGetDeviceQueue(this->device, this->graphicsQueueFamilyIndex, 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, this->presentQueueFamilyIndex, 0, &this->presentQueue);
    return true;
}

bool VulkanContext::createSwapchain(int width, int height)
{
    if (HDR_SWAPCHAIN)
    {
        // The console switches the display to HDR10 when the framebuffers are
        // registered as BGR10A2 BT.2100 PQ; the swapchain is what asks for
        // them. A console or a surface that will not take them falls back to
        // eight bit SDR rather than leaving the title without a picture.
        if (createSwapchainWithFormat(width, height, HDR_SWAPCHAIN_FORMAT))
        {
            Logger::info("vulkan: swapchain is 10bit PQ, the display scans out HDR10");
            return true;
        }
        Logger::error("vulkan: the 10bit PQ swapchain was refused, falling back to 8bit SDR");
    }
    return createSwapchainWithFormat(width, height, VK_FORMAT_B8G8R8A8_UNORM);
}

bool VulkanContext::createSwapchainWithFormat(int width, int height, VkFormat wantedFormat)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physicalDevice, this->surface, &capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    bool offered = false;
    for (const auto& availableFormat : formats)
    {
        if (availableFormat.format == wantedFormat &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = availableFormat;
            offered = true;
            break;
        }
    }
    if (!offered)
    {
        Logger::error("vulkan: the surface does not offer format {}", (int) wantedFormat);
        return false;
    }
    this->swapchainImageFormat = surfaceFormat.format;

    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        extent.width  = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    this->swapchainExtent = extent;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->surface;

    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = {this->graphicsQueueFamilyIndex, this->presentQueueFamilyIndex};
    if (this->graphicsQueueFamilyIndex != this->presentQueueFamilyIndex)
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = VK_PRESENT_MODE_FIFO_KHR; // VSync on
    createInfo.clipped        = VK_TRUE;

    VkResult res = vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapchain);
    if (res != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to create swap chain ({})", (int)res);
        return false;
    }

    vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, nullptr);
    this->swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, this->swapchainImages.data());

    // The painter encodes PQ only when the display scans ten bit PQ out. The
    // mode is shared state (nvgSetHdrOutput writes it), so the frames that
    // follow - the whole user interface included - are encoded the same way.
    nvgSetHdrOutput(nullptr, this->isHdrSwapchain() ? 1 : 0);

    return true;
}

bool VulkanContext::createImageViews()
{
    this->swapchainImageViews.resize(this->swapchainImages.size());

    for (size_t i = 0; i < this->swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                           = this->swapchainImages[i];
        createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                          = this->swapchainImageFormat;
        createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(this->device, &createInfo, nullptr, &this->swapchainImageViews[i]) != VK_SUCCESS)
        {
            Logger::error("vulkan: failed to create image views");
            return false;
        }
    }
    return true;
}

bool VulkanContext::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = this->swapchainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    this->depthFormat = findDepthFormat();
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = this->depthFormat;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    if (vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to create render pass");
        return false;
    }
    return true;
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return 0;
}

VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(this->physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }
    return VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanContext::findDepthFormat()
{
    return findSupportedFormat(
        {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanContext::createDepthResources()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = this->swapchainExtent.width;
    imageInfo.extent.height = this->swapchainExtent.height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = this->depthFormat;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(this->device, &imageInfo, nullptr, &this->depthImage) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(this->device, this->depthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(this->device, &allocInfo, nullptr, &this->depthImageMemory) != VK_SUCCESS)
        return false;

    vkBindImageMemory(this->device, this->depthImage, this->depthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = this->depthImage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = this->depthFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    return (vkCreateImageView(this->device, &viewInfo, nullptr, &this->depthImageView) == VK_SUCCESS);
}

bool VulkanContext::createFramebuffers()
{
    this->swapchainFramebuffers.resize(this->swapchainImageViews.size());

    for (size_t i = 0; i < this->swapchainImageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
            this->swapchainImageViews[i],
            this->depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = this->renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = this->swapchainExtent.width;
        framebufferInfo.height          = this->swapchainExtent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &this->swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            Logger::error("vulkan: failed to create framebuffer");
            return false;
        }
    }
    return true;
}

bool VulkanContext::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = this->graphicsQueueFamilyIndex;

    return (vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool) == VK_SUCCESS);
}

bool VulkanContext::createCommandBuffers()
{
    this->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = this->commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.size());

    return (vkAllocateCommandBuffers(this->device, &allocInfo, this->commandBuffers.data()) == VK_SUCCESS);
}

bool VulkanContext::createSyncObjects()
{
    this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS)
        {
            Logger::error("vulkan: failed to create synchronization objects");
            return false;
        }
    }
    return true;
}

void VulkanContext::cleanupSwapchain()
{
    if (this->depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(this->device, this->depthImageView, nullptr);
        this->depthImageView = VK_NULL_HANDLE;
    }
    if (this->depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(this->device, this->depthImage, nullptr);
        this->depthImage = VK_NULL_HANDLE;
    }
    if (this->depthImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(this->device, this->depthImageMemory, nullptr);
        this->depthImageMemory = VK_NULL_HANDLE;
    }

    for (auto framebuffer : this->swapchainFramebuffers)
    {
        vkDestroyFramebuffer(this->device, framebuffer, nullptr);
    }
    this->swapchainFramebuffers.clear();

    for (auto imageView : this->swapchainImageViews)
    {
        vkDestroyImageView(this->device, imageView, nullptr);
    }
    this->swapchainImageViews.clear();

    if (this->swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
        this->swapchain = VK_NULL_HANDLE;
    }
}

void VulkanContext::recreateSwapchain(int width, int height)
{
    vkDeviceWaitIdle(this->device);
    cleanupSwapchain();

    createSwapchain(width, height);
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

bool VulkanContext::onFramebufferSize(int width, int height, bool init)
{
    if (width == 0 || height == 0) return false;
    this->windowWidth  = width;
    this->windowHeight = height;
    if (!init)
    {
        recreateSwapchain(width, height);
    }
    return true;
}

void VulkanContext::beginFrame()
{
    VkResult fenceRes = vkWaitForFences(this->device, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    if (fenceRes != VK_SUCCESS)
    {
        Logger::error("vulkan: vkWaitForFences failed ({}) on frame {}", (int)fenceRes, this->currentFrame);
        return;
    }

    VkResult result = vkAcquireNextImageKHR(this->device, this->swapchain, UINT64_MAX,
                                            this->imageAvailableSemaphores[this->currentFrame],
                                            VK_NULL_HANDLE, &this->currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain(this->windowWidth, this->windowHeight);
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        Logger::error("vulkan: vkAcquireNextImageKHR failed ({}) on frame {}", (int)result, this->currentFrame);
        return;
    }

    vkResetFences(this->device, 1, &this->inFlightFences[this->currentFrame]);

    VkCommandBuffer cmd = this->commandBuffers[this->currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    if (this->currentImageIndex >= this->swapchainFramebuffers.size() ||
        this->swapchainFramebuffers[this->currentImageIndex] == VK_NULL_HANDLE ||
        this->renderPass == VK_NULL_HANDLE)
    {
        Logger::error("vulkan: invalid swapchain framebuffer or renderPass");
        return;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = this->renderPass;
    renderPassInfo.framebuffer       = this->swapchainFramebuffers[this->currentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->swapchainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(this->swapchainExtent.width);
    viewport.height   = static_cast<float>(this->swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    this->frameStarted = true;
}

void VulkanContext::endFrame()
{
    if (!this->frameStarted) return;
    this->frameStarted = false;

    VkCommandBuffer cmd = this->commandBuffers[this->currentFrame];
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to record command buffer");
        return;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cmd;

    VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]) != VK_SUCCESS)
    {
        Logger::error("vulkan: failed to submit draw command buffer");
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapchains[] = {this->swapchain};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapchains;
    presentInfo.pImageIndices   = &this->currentImageIndex;

    VkResult result = vkQueuePresentKHR(this->presentQueue, &presentInfo);
#if defined(PS5)
    static bool s_splashHidden = false;
    if (!s_splashHidden && result == VK_SUCCESS)
    {
        s_splashHidden = true;
        int rc = sceSystemServiceHideSplashScreen();
        Logger::info("ps5: sceSystemServiceHideSplashScreen called after first Vulkan present (result: {:#x})", rc);
    }
#endif
#if defined(PS5)
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain(this->windowWidth, this->windowHeight);
    }
#else
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain(this->windowWidth, this->windowHeight);
    }
#endif

    this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

} // namespace brls

#endif // BOREALIS_USE_VULKAN
