#include "VulkanRenderer.hpp"
#include "errorHandling.hpp"
#include "Logging.hpp"

#include <filesystem>
#include <fstream>
#include <span>
#include <chrono>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shaderc/shaderc.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace DF
{

VulkanRenderer::VulkanRenderer(Window& wnd) : mWindow{wnd}, mDevice{wnd}
{  
    initCommandPool();
    initSwapChain();
    initImageViews();
    initRenderPass();
    initDescriptorSetLayout();
    initUniformBuffers();
    initDescriptorPool();
    initDepthRescources();
    initTextureImage();
    initTextureImageView();
    initTextureSampler();
    initDescriptorSets();
    initFramebuffers();
    loadModel();
    initVertexBuffer();
    initIndexBuffer();
    initCommandBuffers();
    initSynchronizationObjects();
    initShaderModules();
    initPipelineAndLayout();
}

VulkanRenderer::~VulkanRenderer()
{
    auto device {mDevice.getLogicalDevice()};

    vkDeviceWaitIdle(device);

    cleanupSwapChain();
    vkDestroySampler(device, mTextureSampler, nullptr);
    vkDestroyImageView(device, mTextureImageView, nullptr);
    vkDestroyImage(device, mTextureImage, nullptr);
    vkFreeMemory(device, mTextureImageMemory, nullptr);
    vkDestroyBuffer(device, mVertexBuff, nullptr);
    vkFreeMemory(device, mVertexBuffMemory, nullptr);
    vkDestroyBuffer(device, mIndexBuff, nullptr);
    vkFreeMemory(device, mIndexBuffMemory, nullptr);
    vkDestroyRenderPass(device, mRenderPass, nullptr);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyBuffer(device, mUniformBuffers[i], nullptr);
        vkFreeMemory(device, mUniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
    vkDestroyDescriptorPool(device, mImguiInitInfo.DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
    vkDestroyShaderModule(device, mFragShaderModule, nullptr);
    vkDestroyShaderModule(device, mVertShaderModule, nullptr);
    vkDestroyCommandPool(device, mCommandPool, nullptr);
    vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    vkDestroyPipeline(device, mGraphicsPipeline, nullptr);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(mDevice.getLogicalDevice(), mImageAvailableSem[i], nullptr);
        vkDestroySemaphore(mDevice.getLogicalDevice(), mRenderFinishedSem[i], nullptr);
        vkDestroyFence(mDevice.getLogicalDevice(), mInFlightFence[i], nullptr);
    }
}

void VulkanRenderer::updateUniformBuffer(U32 currentFrame, float dt, float modelAngle)
{
    static float secondsAccumulator{};
    secondsAccumulator += dt;

    MVPMatrices matrices
    {
        .model = glm::rotate(glm::mat4(1.0), modelAngle, glm::vec3(0, 0, 1.0)),

        .view = glm::lookAt(glm::vec3(1.7f, 1.7f, 1.7f),
            glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),

        .proj = glm::perspective(glm::radians(45.0f),
            mSwapChainExtent.width / (float)mSwapChainExtent.height, 0.1f, 10.0f)
    };

    matrices.proj[1][1] *= -1;

    memcpy(mUniformBuffersMapped[currentFrame], &matrices, sizeof matrices);
}

void VulkanRenderer::recordCommands(VkCommandBuffer cmdBuffer, 
    U32 imageIndex, F32 deltaTime, glm::vec<2, double> mousePos)
{
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if(auto res{vkBeginCommandBuffer(cmdBuffer, &beginInfo)}; res != VK_SUCCESS)
        throw DFException{"vkBeginCommandBuffer failed", res};
    
    {//begin the render pass
        std::array<VkClearValue, 2> clearColors {};
        clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearColors[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mRenderPass,
            .framebuffer = mSwapChainFramebuffers[imageIndex],
            .clearValueCount = (U32)clearColors.size(),
            .pClearValues = clearColors.data()
        };
        renderPassInfo.renderArea.extent = mSwapChainExtent;

        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    {
        VkViewport const viewport
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(mSwapChainExtent.width),
            .height = static_cast<float>(mSwapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        
        VkRect2D const scissor
        {
            .offset = {0, 0},
            .extent = mSwapChainExtent
        };
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    }

    {
        VkBuffer vertexBuffers[] = {mVertexBuff};
        VkDeviceSize offsets[] = {0};
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, mIndexBuff, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            mPipelineLayout, 0, 1, &mDescriptorSets[mCurrentFrame], 0, nullptr);
    }

    {
        static F32 incTime{0.0f};
        incTime += deltaTime;

        fragShaderPushConstants pushConstants
        {
            .increasingTimeSeconds = incTime,
            .width = mSwapChainExtent.width,
            .height = mSwapChainExtent.height,
            .mousePosX = (float)mousePos.x,
            .mousePosY = (float)mousePos.y
        };

        vkCmdPushConstants(cmdBuffer, mPipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof fragShaderPushConstants, &pushConstants);
    }

    vkCmdDrawIndexed(cmdBuffer, mIndices.size(), 1, 0, 0, 0);

    ImDrawData* imguiDrawData {ImGui::GetDrawData()};
    ImGui_ImplVulkan_RenderDrawData(imguiDrawData, cmdBuffer);

    vkCmdEndRenderPass(cmdBuffer);

    if(auto res{vkEndCommandBuffer(cmdBuffer)}; res != VK_SUCCESS)
        throw DFException{"vkEndCommandBuffer failed", res};
}

void VulkanRenderer::update(F64 deltaTime, glm::vec<2, double> mousePos, float modelAngle)
{
    auto const device {mDevice.getLogicalDevice()};
    auto const graphicsQueue {mDevice.getGraphicsQueue()};
    
    //wait on mInFlightFence[mCurrentFrame] to be signaled, which indicates that the graphics queue
    //is done with mCommandBuffers[mCurrentFrame], and it can now be recorded to again.
    vkWaitForFences(device, 1, &mInFlightFence[mCurrentFrame], VK_TRUE, UINT64_MAX);
    U32 imageIndex{0};

    {
        auto const res = vkAcquireNextImageKHR(device, mSwapChain, 
            UINT64_MAX, mImageAvailableSem[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

        if(res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw DFException{"vkAcquireNextImageKHR failed", res};

        if(res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
    }
    
    vkResetFences(device, 1, &mInFlightFence[mCurrentFrame]);
    vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);
    recordCommands(mCommandBuffers[mCurrentFrame], imageIndex, deltaTime, mousePos);
    updateUniformBuffer(mCurrentFrame, (float)deltaTime, modelAngle);

    {
        VkPipelineStageFlags const waitStage {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &mImageAvailableSem[mCurrentFrame],
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &mCommandBuffers[mCurrentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &mRenderFinishedSem[mCurrentFrame],
        };

        //submit work on the graphics queue, and once that work is complete singal mInFlightFence[mCurrentFrame]
        if(auto res{vkQueueSubmit(graphicsQueue, 1, &submitInfo, mInFlightFence[mCurrentFrame])}; 
            res != VK_SUCCESS)
        {
            throw DFException{"vkQueueSubmit failed", res};
        }
    }

    {
        VkPresentInfoKHR presentInfo
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &mRenderFinishedSem[mCurrentFrame],
            .swapchainCount = 1,
            .pSwapchains = &mSwapChain,
            .pImageIndices = &imageIndex,
            //.pResults = nullptr
        };

        if(auto res {vkQueuePresentKHR(mDevice.getPresentQueue(), &presentInfo)})
        {
            if(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR
                || mWindow.wasFrameBuffResized())
            {
                mWindow.resetFrameBuffResizedFlag();
                recreateSwapChain();
            }
            else if(res != VK_SUCCESS)
            {
                throw DFException{"vkQueuePresentKHR failed", res};
            }
        }
    }

    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::waitForGPUIdle() const
{
    vkDeviceWaitIdle(mDevice.getLogicalDevice());
}

[[nodiscard]] U32 VulkanRenderer::findMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties physicalMemProperties{};
    vkGetPhysicalDeviceMemoryProperties(mDevice.getPhysicalDevice(), &physicalMemProperties);

    for(U32 i = 0; i < physicalMemProperties.memoryTypeCount; ++i)
    {
        if(typeFilter & (1 << i) &&
            (physicalMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw SystemInitException{"could not find a suitable physical memory type"};
}

void VulkanRenderer::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if( ! tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, sModelFpath.data()) )
        throw SystemInitException{warn + err};

    std::unordered_map<Vertex, U32> uniqueVertices{};
    
    mVertices.reserve(5'000);
    mIndices.reserve(10'000);
    
    for(auto const& shape : shapes)
    {
        for(int i = 0; auto const& index : shape.mesh.indices)
        {
            Vertex vertex
            {
                .pos = 
                {
                    attrib.vertices[3 * index.vertex_index],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                },

                .color = {1.0f, 1.0f, 1.0f},

                .textureCoords =
                {
                    attrib.texcoords[2 * index.texcoord_index],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                }
            };

            if( ! uniqueVertices.contains(vertex) )
            {
                uniqueVertices[vertex] = i++;
                mVertices.push_back(vertex);
            }
            
            mIndices.push_back(uniqueVertices[vertex]);
        }
    }
}

void VulkanRenderer::createImage(U32 width, U32 height, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent =
        {
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    
    auto device {mDevice.getLogicalDevice()};

    if(auto res{vkCreateImage(device, &imageInfo, nullptr, &image)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateImage failed", res};
    }
    
    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    
    if(auto res{vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateMemory failed", res};
    }
    
    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, U32 width, U32 height)
{
    VkCommandBuffer commandBuffer {beginSingleTimeCommands()};

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, 
    VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer {beginSingleTimeCommands()};

    VkImageMemoryBarrier barrier
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = 
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    VkPipelineStageFlags srcStage{};
    VkPipelineStageFlags dstStage{};
    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument{"unsupported layout transition!"};
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage, //source pipeline stage
        dstStage, //dest pipeline stage
        0, //dependency flags
        0, nullptr,   //memory barrier size and array
        0, nullptr,   //buffer mem barrier size and array
        1, &barrier); //image mem barrier size and array

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    auto device {mDevice.getLogicalDevice()};

    VkCommandBuffer commandBuffer {VK_NULL_HANDLE};
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer cmdBuffer)
{
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer
    };

    VkQueue graphicsQueue {mDevice.getGraphicsQueue()};

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(mDevice.getLogicalDevice(), mCommandPool, 1, &cmdBuffer);
}

void VulkanRenderer::initTextureImage()
{
    int texWidth{}, texHeight{}, texChannels{};

    stbi_uc* pixels {stbi_load(sTextureFpath.data(), &texWidth,
        &texHeight, &texChannels, STBI_rgb_alpha)};

    if ( ! pixels )
        throw SystemInitException{"failed to load image with stb_image"};

    VkDeviceSize const imageSize {texWidth * texHeight * 4ULL};

    VkBuffer stagingBuff {VK_NULL_HANDLE};
    VkDeviceMemory stagingBuffMemory {VK_NULL_HANDLE};

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuff, stagingBuffMemory);
      
    auto device {mDevice.getLogicalDevice()};

    void* data {nullptr};
    vkMapMemory(device, stagingBuffMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (U32)imageSize);
    vkUnmapMemory(device, stagingBuffMemory);
    
    createImage(texWidth, texHeight, FORMAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mTextureImage, mTextureImageMemory);

    transitionImageLayout(mTextureImage, FORMAT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(stagingBuff, mTextureImage, (U32)texWidth, (U32)texHeight);

    transitionImageLayout(mTextureImage, FORMAT, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuff, nullptr);
    vkFreeMemory(device, stagingBuffMemory, nullptr);
    stbi_image_free(pixels);
}

void VulkanRenderer::initTextureImageView()
{
    mTextureImageView = createImageView(mTextureImage, FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanRenderer::initTextureSampler()
{
    VkSamplerCreateInfo samplerInfo
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
    };

    auto const& deviceProperties {mDevice.getPhysicalDeviceProperties()};

    //set as high as possible for now
    samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

    //use normalized (0-1) coords
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    auto device {mDevice.getLogicalDevice()};
    if(auto res{vkCreateSampler(device, &samplerInfo, nullptr, &mTextureSampler)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateSampler failed", res};
    }
}

void VulkanRenderer::initImguiRenderInfo()
{
    auto device {mDevice.getLogicalDevice()};

    VkDescriptorPool imguiDescPool {VK_NULL_HANDLE};
    {
        VkDescriptorPoolSize pool_sizes[]
        {
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1
            }
        };

        VkDescriptorPoolCreateInfo pool_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1,
            .poolSizeCount = (U32)std::size(pool_sizes),
            .pPoolSizes = pool_sizes
        };

        auto const res {vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescPool)};
        if(res != VK_SUCCESS)
            throw SystemInitException{"vkCreateDescriptorPool failed to make the imgui desc pool", res};
    }

    /*VkRenderPass imguiRenderPass {VK_NULL_HANDLE};
    {
        VkAttachmentDescription attachment = {};
        attachment.format = wd->SurfaceFormat.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = wd->ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;

        if(auto res {vkCreateRenderPass(device, &info, nullptr, &wd->RenderPass)};
            res != VK_SUCCESS)
        {
            throw SystemInitException{
                "vkCreateRenderPass failed to create the imgui render pass", res};
        }
    }*/

    //pass vulkan info to imgui backend
    auto queueFamilies = mDevice.getQueueFamilyIndices();
    mImguiInitInfo.Instance = mDevice.getInstance();
    mImguiInitInfo.PhysicalDevice = mDevice.getPhysicalDevice();
    mImguiInitInfo.Device = mDevice.getLogicalDevice();
    mImguiInitInfo.QueueFamily = *queueFamilies.graphicsFamilyIndex;
    mImguiInitInfo.Queue = mDevice.getGraphicsQueue();
    mImguiInitInfo.PipelineCache = nullptr;
    mImguiInitInfo.DescriptorPool = imguiDescPool;
    mImguiInitInfo.RenderPass = mRenderPass;
    mImguiInitInfo.Subpass = 0;
    mImguiInitInfo.MinImageCount = 3;
    mImguiInitInfo.ImageCount = mSwapChainImages.size();
    mImguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    mImguiInitInfo.Allocator = nullptr;
    mImguiInitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&mImguiInitInfo);

    ////upload fonts to gpu
    //cb.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    //ImGui_ImplVulkan_CreateFontsTexture();
    //cb.end();
    //vk::SubmitInfo si;
    //si.commandBufferCount = cb.size();
    //si.pCommandBuffers = cb.data();
    //vk::Result res = device->getGraphicsQueue().submit(1, &si, {});
    //device->getGraphicsQueue().waitIdle();

    mImGuiRenderInfoInitialized = true;
}

void VulkanRenderer::initImageViews()
{
    mSwapChainImageViews.resize(mSwapChainImages.size());
    for(int i = 0; VkImage img : mSwapChainImages)
    {
        mSwapChainImageViews[i++] = createImageView(mSwapChainImages[i],
            mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanRenderer::initRenderPass()
{
    auto const device {mDevice.getLogicalDevice()};

    VkAttachmentDescription colorAttachment
    {
        .format = mSwapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentDescription depthAttachment
    {
        .format = findDepthFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorAttachmentRef
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
  
    VkAttachmentReference depthAttachmentRef
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };
    
    VkSubpassDependency subpassDependency
    {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,

        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,

        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,

        .srcAccessMask = 0,

        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | 
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    std::array const attachmentDescriptions {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (U32)attachmentDescriptions.size(),
        .pAttachments = attachmentDescriptions.data(),  
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency
    };

    if(auto res{vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateRenderPass failed ", res};
    }
}

void VulkanRenderer::initSynchronizationObjects()
{
    VkSemaphoreCreateInfo const semCreateInfo {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    auto const device {mDevice.getLogicalDevice()};

    VkFenceCreateInfo const fenceInfo
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,

        //start the fence in the signaled state so the first call to update() does not block indefinitely
        .flags = VK_FENCE_CREATE_SIGNALED_BIT 
    };

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mImageAvailableSem[i])}; 
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateSemaphore failed", res};
        }

        if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mRenderFinishedSem[i])};
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateSemaphore failed", res};
        }

        if(auto res{vkCreateFence(device, &fenceInfo, nullptr, &mInFlightFence[i])}; res != VK_SUCCESS)
            throw SystemInitException{"vkCreateFence failed", res}; 
    }
}

void VulkanRenderer::initDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding combinedImgSamplerLayoutBinding
    {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutBinding uniformBuffLayoutBinding
    {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    std::array const layoutBindings {combinedImgSamplerLayoutBinding, uniformBuffLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (U32)layoutBindings.size(),
        .pBindings = layoutBindings.data()
    };

    auto device {mDevice.getLogicalDevice()};
    if(auto res{vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayout)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateDescriptorSetLayout failed", res};
    }
}

void VulkanRenderer::initUniformBuffers()
{
    VkDeviceSize const buffSize {sizeof(MVPMatrices)};
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(buffSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mUniformBuffers[i], mUniformBuffersMemory[i]);

        //The UBO is mapped for the lifetime of the renderer.
        auto device {mDevice.getLogicalDevice()};
        vkMapMemory(device, mUniformBuffersMemory[i], 0, buffSize, 0, &mUniformBuffersMapped[i]);
    }
}

void VulkanRenderer::initDescriptorSets()
{
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(mDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = (U32)(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data(),
    };

    auto device {mDevice.getLogicalDevice()};
    if(auto res{vkAllocateDescriptorSets(device, &allocInfo, mDescriptorSets.data())};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to allocate descriptor sets!", res};
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo bufferInfo
        {
            .buffer = mUniformBuffers[i],
            .offset = 0,
            .range = sizeof(MVPMatrices)
        };

        VkDescriptorImageInfo imageInfo
        {
            .sampler = mTextureSampler,
            .imageView = mTextureImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        std::array const descriptorWrites
        {
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo
            },
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo
            },
        };

        vkUpdateDescriptorSets(device, (U32)descriptorWrites.size(), 
            descriptorWrites.data(), 0, nullptr);
    }
}

void VulkanRenderer::initDescriptorPool()
{
    std::array const poolSizes
    {
        VkDescriptorPoolSize
        {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            (U32)(MAX_FRAMES_IN_FLIGHT),
        },
        VkDescriptorPoolSize
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            (U32)(MAX_FRAMES_IN_FLIGHT),
        }
    };

    VkDescriptorPoolCreateInfo poolInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = (U32)(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = (U32)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    auto device {mDevice.getLogicalDevice()};
    if(auto res{vkCreateDescriptorPool(device, &poolInfo, nullptr, &mDescriptorPool)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateDescriptorPool failed", res};
    }
}

void VulkanRenderer::initCommandBuffers()
{
    VkCommandBufferAllocateInfo const commandBuffAllocInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<U32>(mCommandBuffers.size())
    };

    if(auto res{vkAllocateCommandBuffers(mDevice.getLogicalDevice(), 
        &commandBuffAllocInfo, mCommandBuffers.data())}; res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateCommandBuffers failed", res};
    }
}

void VulkanRenderer::initCommandPool()
{
    auto const device {mDevice.getLogicalDevice()};
    auto queueFamilyIndices {mDevice.getQueueFamilyIndices()};

    VkCommandPoolCreateInfo commandPoolCreationInfo 
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                 VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = *queueFamilyIndices.graphicsFamilyIndex,
    };

    if(auto res{vkCreateCommandPool(device, &commandPoolCreationInfo, nullptr, &mCommandPool)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateCommandPool failed", res};
    }

}

void VulkanRenderer::cleanupSwapChain()
{
    auto device { mDevice.getLogicalDevice() };

    vkDestroyImageView(device, mDepthImageView, nullptr);
    vkDestroyImage(device, mDepthImage, nullptr);
    vkFreeMemory(device, mDepthImageMemory, nullptr);

    for(auto framebuffer : mSwapChainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    for(auto imageView : mSwapChainImageViews)
        vkDestroyImageView(device, imageView, nullptr);

    vkDestroySwapchainKHR(device, mSwapChain, nullptr);
}

void VulkanRenderer::recreateSwapChain()
{
    auto frameBuffSize = mWindow.getFBSize();
    while(frameBuffSize.x == 0 || frameBuffSize.y == 0)
    {
        frameBuffSize = mWindow.getFBSize();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(mDevice.getLogicalDevice());
    initSwapChain();
    initImageViews();
    initDepthRescources();
    initFramebuffers();
}

void VulkanRenderer::initFramebuffers()
{
    mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

    for(size_t i = 0; i < mSwapChainImageViews.size(); i++) 
    {
        std::array const attachments {mSwapChainImageViews[i], mDepthImageView};

        VkFramebufferCreateInfo framebufferInfo
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mRenderPass,
            .attachmentCount = attachments.size(),
            .pAttachments = attachments.data(),
            .width = mSwapChainExtent.width,
            .height = mSwapChainExtent.height,
            .layers = 1
        };

        if(auto res{vkCreateFramebuffer(mDevice.getLogicalDevice(), &framebufferInfo, nullptr, 
            &mSwapChainFramebuffers[i])}; res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateFramebuffer failed", res};
        }
    }
}

//helper func for createSwapChain
[[nodiscard]]
static VkExtent2D chooseSwapChainExtent(VkSurfaceCapabilitiesKHR const& surfaceCapabilities, GLFWwindow* window) 
{
    if(surfaceCapabilities.currentExtent.width != std::numeric_limits<U32>::max())
        return surfaceCapabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    VkExtent2D actualExtent
    {
        static_cast<U32>(width),
        static_cast<U32>(height)
    };
    
    actualExtent.width = std::clamp(actualExtent.width,
        surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
        surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    
    return actualExtent;
}

//helper func for createSwapChain.
//in the future the present mode will be modifiable when toggling vsync
[[nodiscard]]
static VkPresentModeKHR chooseSwapChainPresentMode(std::span<const VkPresentModeKHR> availableModes)
{
    for(const auto& availablePresentMode : availableModes)
    {
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR; //FIFO is guaranteed to be available
}

//helper func for createSwapChain
[[nodiscard]]
static VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(std::span<const VkSurfaceFormatKHR> availableFormats)
{
    assert( ! availableFormats.empty() );

    for(auto const& availableFormat : availableFormats) 
    {
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }   
    
    return availableFormats[0];
}

void VulkanRenderer::initSwapChain()
{
    VulkanDevice::SwapChainSupportDetails swapChainSupportInfo;
    mDevice.getSwapChainSupportInfo(swapChainSupportInfo);

    VkSurfaceFormatKHR surfaceFormat {chooseSwapChainSurfaceFormat(swapChainSupportInfo.formats)};
    VkPresentModeKHR presentMode {chooseSwapChainPresentMode(swapChainSupportInfo.presentModes)};
    VkExtent2D extent {chooseSwapChainExtent(swapChainSupportInfo.capabilities, mWindow.getRawWindow())};

    U32 numOfSwapChainImages { swapChainSupportInfo.capabilities.minImageCount  + 1 };

    //an image count of 0 inside the swap chain support info means that there is 
    //no maximum amount of images we can have in our swap chain
    if(swapChainSupportInfo.capabilities.maxImageCount > 0 && 
        numOfSwapChainImages > swapChainSupportInfo.capabilities.maxImageCount) 
    {
        numOfSwapChainImages = swapChainSupportInfo.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreationInfo
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = mDevice.getSurface(),
        .minImageCount = numOfSwapChainImages,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainSupportInfo.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = mSwapChain,
    };

    VulkanDevice::QueueFamilyIndices indices {mDevice.getQueueFamilyIndices()};
    U32 queueFamilyIndices[2] {*indices.graphicsFamilyIndex, *indices.presentFamilyIndex};

    if(*indices.graphicsFamilyIndex != *indices.presentFamilyIndex)
    {
        swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreationInfo.queueFamilyIndexCount = 2;
        swapChainCreationInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    auto const device { mDevice.getLogicalDevice() };

    if(auto res{vkCreateSwapchainKHR(device, &swapChainCreationInfo, nullptr, &mSwapChain)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateSwapchainKHR failed", res};
    }

    vkGetSwapchainImagesKHR(device, mSwapChain, &numOfSwapChainImages, nullptr);
    mSwapChainImages.resize(numOfSwapChainImages);
    vkGetSwapchainImagesKHR(device, mSwapChain, &numOfSwapChainImages, mSwapChainImages.data());

    mSwapChainImageFormat = surfaceFormat.format;
    mSwapChainExtent = extent;
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory)
{
    auto const device {mDevice.getLogicalDevice()};

    VkBufferCreateInfo bufferInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        //.flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if(auto res{vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateBuffer failed", res};
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);

    U32 memType {findMemoryType(memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    VkMemoryAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memType
    };

    if(auto res {vkAllocateMemory(device, &allocInfo, nullptr, &outMemory)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateMemory failed", res};
    }

    if(auto res {vkBindBufferMemory(device, outBuffer, outMemory, 0)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkBindBufferMemory failed", res};
    }
}

void VulkanRenderer::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    auto const device {mDevice.getLogicalDevice()};

    VkCommandBuffer cmdBuff {beginSingleTimeCommands()};

    VkBufferCopy copyRegion {.size = size};
    vkCmdCopyBuffer(cmdBuff, src, dst, 1, &copyRegion);
    vkEndCommandBuffer(cmdBuff);

    VkSubmitInfo submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuff
    };

    auto graphicsQueue {mDevice.getGraphicsQueue()};
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, mCommandPool, 1, &cmdBuff);
}

void VulkanRenderer::initIndexBuffer()
{
    auto const device {mDevice.getLogicalDevice()};
    VkDeviceSize buffSize {sizeof(mIndices[0]) * mIndices.size()};
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    void* data {nullptr};
    vkMapMemory(device, stagingBufferMemory, 0, buffSize, 0, &data);
    memcpy(data, mIndices.data(), (size_t)buffSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuff, mIndexBuffMemory);

    copyBuffer(stagingBuffer, mIndexBuff, buffSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
        {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    auto const device {mDevice.getLogicalDevice()};
    VkImageView imageView {VK_NULL_HANDLE};

    if(auto res{vkCreateImageView(device, &viewInfo, nullptr, &imageView)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to create texture image view!"};
    }

    return imageView;
}

static bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanRenderer::findDepthFormat() 
{
    std::array const candidateFormats
    {
        VK_FORMAT_D32_SFLOAT, 
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    return findSupportedFormat(candidateFormats, VK_IMAGE_TILING_OPTIMAL, 
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanRenderer::findSupportedFormat(std::span<const VkFormat> candidates, 
    VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for(VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(mDevice.getPhysicalDevice(), format, &props);

        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    throw SystemInitException{"failed to find a supported format"};
}

void VulkanRenderer::initDepthRescources()
{
    VkFormat const depthFormat {findDepthFormat()};

    createImage(mSwapChainExtent.width, mSwapChainExtent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);

    mDepthImageView = createImageView(mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::initVertexBuffer()
{
    VkDeviceSize buffSize {sizeof(mVertices[0]) * mVertices.size()};
    VkBuffer stagingBuff;
    VkDeviceMemory stagingBuffMemory;

    createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuff, stagingBuffMemory);

    auto device {mDevice.getLogicalDevice()};

    void* mappedData {nullptr};
    vkMapMemory(device, stagingBuffMemory, 0, buffSize, 0, &mappedData);
    std::memcpy(mappedData, mVertices.data(), buffSize);
    vkUnmapMemory(device, stagingBuffMemory);

    createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuff, mVertexBuffMemory);

    copyBuffer(stagingBuff, mVertexBuff, buffSize);

    vkDestroyBuffer(device, stagingBuff, nullptr);
    vkFreeMemory(device, stagingBuffMemory, nullptr);
}

[[nodiscard]]
static VkShaderModule createShaderModule(U32 const* const spirVCode,
    size_t spirVSize, VkDevice device)
{
    VkShaderModuleCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirVSize,
        .pCode = spirVCode
    };

    VkShaderModule retval;
    if(auto res{vkCreateShaderModule(device, &createInfo, nullptr, &retval)}; res != VK_SUCCESS)
    {   
        std::string failureReason {std::format("vKCreateShaderModule failed. VkResult = {}", static_cast<int>(res))};
        Logger::get().stdoutError(failureReason);
        return VK_NULL_HANDLE;
    }

    return retval;
}

[[nodiscard]]
static std::string getFileAsString(std::filesystem::path const& fname)
{
    std::ifstream ifs{fname, std::ios_base::ate | std::ios_base::binary};

    if( ! ifs )
    {
        throw DFException{ std::string{"could not open "}.append(fname.string()) };
    }

    std::string retval;
    retval.resize(ifs.tellg());
    ifs.seekg(0);
    ifs.read(retval.data(), retval.size());

    return retval;
}

[[nodiscard]] //returns VK_NULL_HANDLE if something went wrong
static VkShaderModule compileGLSLShaders(VkDevice device, 
    std::string_view shaderPath, shaderc_shader_kind shaderType)
{
    std::string const glslSource {getFileAsString(shaderPath)};

    shaderc::Compiler const compiler;

    shaderc::SpvCompilationResult compilationResult {
        compiler.CompileGlslToSpv(glslSource, shaderType, shaderPath.data()) };

    if(auto res {compilationResult.GetCompilationStatus()}; res != shaderc_compilation_status_success)
    {
        Logger::get().stdoutError(std::string{"Could not compile "}.append(shaderPath));
        Logger::get().stdoutError(compilationResult.GetErrorMessage());
        return VK_NULL_HANDLE;
    }
    
    //get the size of the code in bytes
    size_t spirVlen {0};
    for(U32 [[maybe_unused]] dword : compilationResult)
        spirVlen += sizeof U32;

    return DF::createShaderModule(compilationResult.cbegin(), spirVlen, device);
}

void VulkanRenderer::initShaderModules()
{
    auto const device {mDevice.getLogicalDevice()};

    Logger::get().stdoutInfo("Compiling shaders...");

    const auto start = std::chrono::steady_clock::now();

    //TODO directory iterator and compile anything that has the right extension and also check time stamps
    mVertShaderModule = compileGLSLShaders(device,
        "resources/shaders/triangleTest.vert", shaderc_vertex_shader);

    if(mVertShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    mFragShaderModule = compileGLSLShaders(device, 
        "resources/shaders/triangleTest.frag", shaderc_fragment_shader);

    if(mFragShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    const auto end = std::chrono::steady_clock::now();

    Logger::get().fmtStdoutWarn("compiling glsl to Spir-V took {} seconds", 
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1.0E9);
}

void VulkanRenderer::initPipelineAndLayout()
{
    std::array shaderStages
    {
        VkPipelineShaderStageCreateInfo
        {//vertex
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = mVertShaderModule,
            .pName = "main"
        },
        VkPipelineShaderStageCreateInfo
        {//fragment
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = mFragShaderModule,
            .pName = "main"
        }
    };

    VkVertexInputBindingDescription const bindingDescription{
        Vertex::getBindingDescription()};

    auto const attributeDescriptions {Vertex::getAttributeDescriptions()};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = (U32)attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    std::array const dynamicStates {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (U32)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport const viewport
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)mSwapChainExtent.width,
        .height = (float)mSwapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D const scissor {.offset = {0, 0}, .extent = mSwapChainExtent};

    VkPipelineViewportStateCreateInfo viewportState
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo const rasterizer
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        //.depthClampEnable = VK_FALSE,
        //.rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        //.depthBiasEnable = VK_FALSE,
        //.depthBiasConstantFactor = 0.0f,
        //.depthBiasClamp = 0.0f, 
        //.depthBiasSlopeFactor = 0.0f, 
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo const multisampling
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        //.sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        //.pSampleMask = nullptr,
        //.alphaToCoverageEnable = VK_FALSE,
        //.alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState const colorBlendAttachment
    {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
            VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT | 
            VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        //.logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkPushConstantRange const pushConstantRange
    {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof fragShaderPushConstants
    };

    VkPipelineLayoutCreateInfo const pipelineLayoutInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    auto const device {mDevice.getLogicalDevice()};

    if(auto res{vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vKCreatePipelineLayout failed ", res};
    }

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        //.depthBoundsTestEnable = VK_FALSE,
        //.minDepthBounds = 0.0f,
        //.maxDepthBounds = 1.0f,
        //.stencilTestEnable = VK_FALSE,
        //depthStencil.front = {},
        //depthStencil.back = {}
    };

    VkGraphicsPipelineCreateInfo const pipelineInfo
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = mPipelineLayout,
        .renderPass = mRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if(auto res{vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
        &pipelineInfo, nullptr, &mGraphicsPipeline)}; res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to create graphics pipeline!"};
    }
}

}//end namespace DF