#pragma once
#include <vector>
#include <array>
#include "VulkanDevice.hpp"
#include "HelpfulTypeAliases.hpp"
#include <glm/glm.hpp>
#include <imgui_impl_vulkan.h>

class Window;

namespace DF
{

class VulkanRenderer
{
public:
    
    VulkanRenderer(Window& wnd);
    ~VulkanRenderer();

    void initImguiRenderInfo();

    void update(F64 deltaTime, glm::vec<2, double> mousePos);

private:

    //used as a uniform buffer object
    struct MVPMatrices
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        static auto getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription
            {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };

            return bindingDescription;
        }
        
        static auto getAttributeDescriptions() 
        {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };

    constexpr static U32 MAX_FRAMES_IN_FLIGHT {2};
    U32 mCurrentFrame {0};

    std::array<Vertex, 4> const mVertices
    {
        Vertex{ {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        Vertex{ { 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        Vertex{ { 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        Vertex{ {-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    std::array<U32, 6> const mIndices {0, 1, 2, 2, 3, 0};

    VkBuffer mVertexBuff {VK_NULL_HANDLE};
    VkBuffer mIndexBuff {VK_NULL_HANDLE};
    VkDeviceMemory mVertexBuffMemory {VK_NULL_HANDLE};
    VkDeviceMemory mIndexBuffMemory {VK_NULL_HANDLE};

    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> mUniformBuffers;
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mUniformBuffersMemory;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> mUniformBuffersMapped;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDescriptorSets;

    Window& mWindow;

    VulkanDevice mDevice;

    VkSwapchainKHR mSwapChain {VK_NULL_HANDLE};
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    std::vector<VkImageView> mSwapChainImageViews;
    std::vector<VkImage> mSwapChainImages;
    VkFormat mSwapChainImageFormat{};
    VkExtent2D mSwapChainExtent{};

    VkRenderPass mRenderPass {VK_NULL_HANDLE};
    VkDescriptorPool mDescriptorPool{VK_NULL_HANDLE};
    
    struct fragShaderPushConstants
    {
        float increasingTimeSeconds{};
        U32 width{};
        U32 height{};
        float mousePosX{};
        float mousePosY{};
    };

    VkDescriptorSetLayout mDescriptorSetLayout {VK_NULL_HANDLE};
    VkPipelineLayout mPipelineLayout {VK_NULL_HANDLE};
    VkPipeline mGraphicsPipeline {VK_NULL_HANDLE};
    VkShaderModule mVertShaderModule {VK_NULL_HANDLE};
    VkShaderModule mFragShaderModule {VK_NULL_HANDLE};

    VkCommandPool mCommandPool {VK_NULL_HANDLE};
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mCommandBuffers;

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSem;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mRenderFinishedSem;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mInFlightFence;

    //call void initImguiRenderInfo() to initialize imgui rendering related things
    bool mImGuiRenderInfoInitialized {false};
    ImGui_ImplVulkan_InitInfo mImguiInitInfo{};

    VkImage mTextureImage {VK_NULL_HANDLE};
    VkDeviceMemory mTextureImageMemory {VK_NULL_HANDLE};
    VkImageView mTextureImageView {VK_NULL_HANDLE};
    VkSampler mTextureSampler {VK_NULL_HANDLE};

    void updateUniformBuffer(U32 currentFrame, float dt);

    void recordCommands(VkCommandBuffer commandBuffer, 
        U32 imageIndex, F32 deltaTime, glm::vec<2, double> mousePos);

    void initPipeline();
    void initImageViews();
    void initRenderPass();
    void initCommandPool();
    void initCommandBuffers();
    void initFramebuffers();
    void initSwapChain();
    void initSynchronizationObjects();
    void initDescriptorSetLayout();
    void initUniformBuffers();
    void initDescriptorSets();
    void initDescriptorPool();
    void initTextureImage();
    void initTextureImageView();
    void initTextureSampler();
    void initVertexBuffer();
    void initIndexBuffer();

    void cleanupSwapChain();
    void recreateSwapChain();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmdBuffer);
    U32 findMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties);
    void createImage(U32 width, U32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void copyBufferToImage(VkBuffer buffer, VkImage image, U32 width, U32 height);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory);
    
public:
    VulkanRenderer(VulkanRenderer const&)=delete;
    VulkanRenderer(VulkanRenderer&&)=delete;
    VulkanRenderer& operator=(VulkanRenderer const&)=delete;
    VulkanRenderer& operator=(VulkanRenderer&&)=delete;
};

}