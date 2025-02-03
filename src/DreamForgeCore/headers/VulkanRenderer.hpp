#pragma once
#include <vector>
#include "VulkanDevice.hpp"

class Window;

namespace DF
{

class VulkanRenderer
{
public:
    
    VulkanRenderer(Window const& wnd);
    ~VulkanRenderer();

    void update();

private:

    Window const& mWindow;

    VulkanDevice mDevice;

    VkSwapchainKHR mSwapChain;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    std::vector<VkImageView> mSwapChainImageViews;
    std::vector<VkImage> mSwapChainImages;
    VkFormat mSwapChainImageFormat{};
    VkExtent2D mSwapChainExtent{};

    VkRenderPass mRenderPass {VK_NULL_HANDLE};
    
    VkPipelineLayout mPipelineLayout   {VK_NULL_HANDLE};
    VkPipeline       mGraphicsPipeline {VK_NULL_HANDLE};
    VkShaderModule   mVertShaderModule {VK_NULL_HANDLE};
    VkShaderModule   mFragShaderModule {VK_NULL_HANDLE};

    VkCommandPool mCommandPool {VK_NULL_HANDLE};
    VkCommandBuffer mCommandBuffer {VK_NULL_HANDLE};

    VkSemaphore mImageAvailableSem {VK_NULL_HANDLE};
    VkSemaphore mRenderFinishedSem {VK_NULL_HANDLE};
    VkFence mInFlightFence {VK_NULL_HANDLE};

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //init shader modules, graphics pipeline, and pipeline layout
    void createPipeline();

    void createImageViews();
    void createRenderPass();
    void createCommandPool();
    void createCommandBuffer();
    void createFramebuffers();
    void createSwapChain();
    void createSynchronizationObjects();
    
public:
    VulkanRenderer(VulkanRenderer const&)=delete;
    VulkanRenderer(VulkanRenderer&&)=delete;
    VulkanRenderer& operator=(VulkanRenderer const&)=delete;
    VulkanRenderer& operator=(VulkanRenderer&&)=delete;
};

}