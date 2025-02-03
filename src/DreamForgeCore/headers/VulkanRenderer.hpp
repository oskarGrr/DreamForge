#pragma once
#include <vector>
#include <array>
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

    constexpr static U32 MAX_FRAMES_IN_FLIGHT {2};
    U32 mCurrentFrame {0};

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
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mCommandBuffers;

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSem;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mRenderFinishedSem;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mInFlightFence;

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    
    void initPipeline();
    void initImageViews();
    void initRenderPass();
    void initCommandPool();
    void initCommandBuffers();
    void initFramebuffers();
    void initSwapChain();
    void createSynchronizationObjects();
    
public:
    VulkanRenderer(VulkanRenderer const&)=delete;
    VulkanRenderer(VulkanRenderer&&)=delete;
    VulkanRenderer& operator=(VulkanRenderer const&)=delete;
    VulkanRenderer& operator=(VulkanRenderer&&)=delete;
};

}