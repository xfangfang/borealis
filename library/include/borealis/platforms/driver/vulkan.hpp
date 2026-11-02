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

#pragma once

#include <nanovg.h>
#include <nanovg_vk.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <string>

#ifdef __SDL2__
#include <SDL2/SDL.h>
#endif

namespace brls
{

class VulkanContext
{
public:
#ifdef __SDL2__
    VulkanContext(SDL_Window* window, int width, int height);
#endif
    ~VulkanContext();

    double getScaleFactor();
    void clear(NVGcolor color);
    void beginFrame();
    void endFrame();
    void setSwapInterval(int value);

    bool onFramebufferSize(int width, int height, bool init = false);

    VkInstance getInstance() const { return this->instance; }
    VkPhysicalDevice getPhysicalDevice() const { return this->physicalDevice; }
    VkDevice getDevice() const { return this->device; }
    VkQueue getGraphicsQueue() const { return this->graphicsQueue; }
    uint32_t getGraphicsQueueFamilyIndex() const { return this->graphicsQueueFamilyIndex; }
    VkRenderPass getRenderPass() const { return this->renderPass; }
    VkCommandPool getCommandPool() const { return this->commandPool; }
    VkCommandBuffer getCurrentCommandBuffer() const { return this->commandBuffers[this->currentFrame]; }
    uint32_t getCurrentFrame() const { return this->currentFrame; }
    uint32_t getSwapchainImageCount() const { return static_cast<uint32_t>(this->swapchainImages.size()); }
    VkSwapchainKHR getSwapchain() const { return this->swapchain; }
    VkExtent2D getSwapchainExtent() const { return this->swapchainExtent; }

    VKNVGCreateInfo getNVGCreateInfo();

    static const int MAX_FRAMES_IN_FLIGHT = 2;

private:
    bool initVulkan(int width, int height);
    void unInitVulkan();

    bool createInstance();
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSurface();
    bool createSwapchain(int width, int height);
    bool createImageViews();
    bool createRenderPass();
    bool createDepthResources();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();

    void cleanupSwapchain();
    void recreateSwapchain(int width, int height);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    SDL_Window* window       = nullptr;
    int windowWidth          = 0;
    int windowHeight         = 0;
    double scaleFactor       = 1.0;
    int swapInterval         = 1;

    VkInstance instance                     = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice         = VK_NULL_HANDLE;
    VkDevice device                         = VK_NULL_HANDLE;
    VkQueue graphicsQueue                   = VK_NULL_HANDLE;
    VkQueue presentQueue                    = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex       = 0;
    uint32_t presentQueueFamilyIndex        = 0;

    VkSurfaceKHR surface                    = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain                = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat           = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent              = {0, 0};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;

public:
    /// Ask for ten bits a channel with PQ, so the display scans out HDR10.
    ///
    /// Set before the platform is created. The swapchain falls back to eight
    /// bit SDR when the surface does not offer the format or the console
    /// refuses to register the buffers for it; isHdrSwapchain() says which
    /// one the frames are drawn into.
    inline static bool HDR_SWAPCHAIN = false;
    /// The ten bit format the scanout takes: VideoOut's Bgr10A2 has blue in the
    /// low bits, so this is A2R10G10B10 (A2B10G10R10 would swap red and blue).
    inline static const VkFormat HDR_SWAPCHAIN_FORMAT = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    bool isHdrSwapchain() const { return this->swapchainImageFormat == HDR_SWAPCHAIN_FORMAT; }

private:
    bool createSwapchainWithFormat(int width, int height, VkFormat format);

    VkImage depthImage                      = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory         = VK_NULL_HANDLE;
    VkImageView depthImageView              = VK_NULL_HANDLE;
    VkFormat depthFormat                    = VK_FORMAT_D24_UNORM_S8_UINT;

    VkRenderPass renderPass                 = VK_NULL_HANDLE;
    VkCommandPool commandPool               = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame                   = 0;
    uint32_t currentImageIndex              = 0;
    NVGcolor clearColor                     = {0.0f, 0.0f, 0.0f, 1.0f};
    bool frameStarted                       = false;
};

extern std::unique_ptr<VulkanContext> VULKAN_CONTEXT;

} // namespace brls
