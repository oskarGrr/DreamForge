#pragma once
#include <vector>
#include <span>
#include <array>
#include <string_view>

#include "VulkanDevice.hpp"
#include "HelpfulTypeAliases.hpp"
#include <glm/glm.hpp>
#include <imgui_impl_vulkan.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

class Window;

struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static constexpr size_t PARTICLE_COUNT {256 * 100};

    static VkVertexInputBindingDescription getBindingDescription()
    {
        return
        {
            .binding = 0,
            .stride = sizeof Particle,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        return
        {{
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Particle, position)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(Particle, color)
            }
        }};
    }
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 textureCoords;

    bool operator<=>(const Vertex&) const = default;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        return
        {
            .binding = 0,
            .stride = sizeof Vertex,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }
    
    static auto getAttributeDescriptions() 
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, textureCoords);

        return attributeDescriptions;
    }
};

namespace std 
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& v) const 
        {
            return ((hash<glm::vec3>()(v.pos) ^
                (hash<glm::vec3>()(v.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(v.textureCoords) << 1);
        }
    };
}

namespace DF
{

class VulkanRenderer
{
public:
    
    VulkanRenderer(Window& wnd);
    ~VulkanRenderer();

    void initImguiRenderInfo();

    void update(F64 deltaTime, glm::vec<2, double> mousePos, float modelAngle);

    void waitForGPUIdle() const;

private:

    VulkanDevice mDevice;

    struct MVPMatrices
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    constexpr static U32 MAX_FRAMES_IN_FLIGHT {2};
    U32 mCurrentFrame {0};

    std::vector<Vertex> mVertices;
    std::vector<U32> mIndices;

    struct ComputeUniformBuffer
    {
        float deltaTime;
    };

    VkBuffer mVertexBuff {VK_NULL_HANDLE};
    VkBuffer mIndexBuff {VK_NULL_HANDLE};
    VkDeviceMemory mVertexBuffMemory {VK_NULL_HANDLE};
    VkDeviceMemory mIndexBuffMemory {VK_NULL_HANDLE};

    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> mComputeUniformBuffers {};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mComputeUniformBuffersMemory {};
    std::array<void*, MAX_FRAMES_IN_FLIGHT> mComputeUniformBuffersMapped {};
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> mUniformBuffers {};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mUniformBuffersMemory {};
    std::array<void*, MAX_FRAMES_IN_FLIGHT> mUniformBuffersMapped {};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDescriptorSets {};

    Window& mWindow;

    VkSwapchainKHR mSwapChain {VK_NULL_HANDLE};
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    std::vector<VkImageView> mSwapChainImageViews;
    std::vector<VkImage> mSwapChainImages;
    VkFormat mSwapChainImageFormat{};
    VkExtent2D mSwapChainExtent{};

    VkRenderPass mRenderPass {VK_NULL_HANDLE};
    VkDescriptorPool mDescriptorPool {VK_NULL_HANDLE};
     
    struct fragShaderPushConstants
    {
        float increasingTimeSeconds{};
        U32 width{};
        U32 height{};
        float mousePosX{};
        float mousePosY{};
    };

    VkImage mColorImage {VK_NULL_HANDLE};
    VkDeviceMemory mColorImageMemory {VK_NULL_HANDLE};
    VkImageView mColorImageView {VK_NULL_HANDLE};

    VkDescriptorSetLayout mDescriptorSetLayout {VK_NULL_HANDLE};
    VkDescriptorSetLayout mComputeDescriptorSetLayout {VK_NULL_HANDLE};
    VkPipelineLayout mPipelineLayout {VK_NULL_HANDLE};
    VkPipelineLayout mComputePipelineLayout {VK_NULL_HANDLE};
    VkPipeline mGraphicsPipeline {VK_NULL_HANDLE};
    VkPipeline mComputePipeline {VK_NULL_HANDLE};
    VkShaderModule mVertShaderModule {VK_NULL_HANDLE};
    VkShaderModule mFragShaderModule {VK_NULL_HANDLE};
    VkShaderModule mComputeShaderModule {VK_NULL_HANDLE};

    VkCommandPool mCommandPool {VK_NULL_HANDLE};
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mComputeCommandBuffers {};
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mCommandBuffers {};

    //synchronization objects
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mComputeInFlightFences {};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mComputeFinishedSemaphores {};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores {};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mRenderFinishedSemaphores {};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mInFlightFence {};

    //call void initImguiRenderInfo() to initialize imgui rendering related things
    bool mImGuiRenderInfoInitialized {false};
    ImGui_ImplVulkan_InitInfo mImguiInitInfo{};

    U32 mMipMapLevels {1};
    VkImage mTextureImage {VK_NULL_HANDLE};
    VkDeviceMemory mTextureImageMemory {VK_NULL_HANDLE};
    VkImageView mTextureImageView {VK_NULL_HANDLE};
    VkSampler mTextureSampler {VK_NULL_HANDLE};

    VkImage mDepthImage {VK_NULL_HANDLE};
    VkDeviceMemory mDepthImageMemory {VK_NULL_HANDLE};
    VkImageView mDepthImageView {VK_NULL_HANDLE};

    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mComputeDescriptorSets {};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mShaderStorageBuffersMemory {};
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> mShaderStorageBuffers {};

    VkSampleCountFlagBits mMSAASampleCount {mDevice.getMaxMsaaSampleCount()};

    void updateUniformBuffer(U32 currentFrame, float dt, float modelAngle);
    void updateComputeUniformBuffer(U32 currentFrame, float dt);

    void recordComputeCommands(VkCommandBuffer cmdBuff);
    void recordCommands(VkCommandBuffer commandBuffer,
        U32 imageIndex, F32 deltaTime, glm::vec<2, double> mousePos);

    inline static std::string_view const sModelFpath = "resources/models/viking_room.obj";
    inline static std::string_view const sTextureFpath = "resources/textures/viking_room.png";

    void initPipelineAndLayout();
    void initShaderModules();
    void initSwapChainImageViews();
    void initRenderPass();
    void initCommandPool();
    void initCommandBuffers();
    void initComputeCommandBuffers();
    void initFramebuffers();
    void initSwapChain();
    void initSynchronizationObjects();
    void initComputeDescriptorSetLayout();
    void initDescriptorSetLayout();
    void initComputeUniformBuffers();
    void initShaderStorageBuffers();
    void initUniformBuffers();
    void initDescriptorSets();
    void initDescriptorPool();
    void initTextureImage();
    void initTextureImageView();
    void initTextureSampler();
    void initComputeDescriptorSets();
    void initVertexBuffer();
    void initIndexBuffer();
    void initDepthRescources();
    void initColorResources();

    void cleanupSwapChain();
    void recreateSwapChain();

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, U32 mipLevels);

    VkFormat findSupportedFormat(std::span<const VkFormat> candidates,
        VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmdBuffer);

    void genMipMaps(VkImage img, VkFormat imgFmt, U32 width, U32 height, U32 mipLevels);

    U32 findMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties);

    void loadModel();

    void createImage(U32 width, U32 height, U32 mipLevels, VkFormat format, 
        VkSampleCountFlagBits numSamples,VkImageTiling tiling, VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void copyBufferToImage(VkBuffer buffer, VkImage image, U32 width, U32 height);

    void transitionImageLayout(VkImage image, VkFormat format, 
        VkImageLayout oldLayout, VkImageLayout newLayout, U32 mipLevels);

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