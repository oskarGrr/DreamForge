#include "VulkanRenderer.hpp"
#include "errorHandling.hpp"
#include "Logging.hpp"

#include <filesystem>
#include <fstream>
#include <span>
#include <chrono>
#include <random>
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
    initSwapChainImageViews();
    initRenderPass();
    initDescriptorSetLayout();
    initComputeDescriptorSetLayout();
    initUniformBuffers();
    initDescriptorPool();
    initColorResources();
    initDepthRescources();
    initTextureImage();
    initTextureImageView();
    initTextureSampler();
    initDescriptorSets();
    initComputeUniformBuffers();
    initShaderStorageBuffers();
    initComputeDescriptorSets();
    initFramebuffers();
    loadModel();
    initVertexBuffer();
    initIndexBuffer();
    initCommandBuffers();
    initImguiCommandBuffers();
    initComputeCommandBuffers();
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
    vkDestroyRenderPass(device, mImguiRenderPass, nullptr);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyBuffer(device, mUniformBuffers[i], nullptr);
        vkFreeMemory(device, mUniformBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, mComputeUniformBuffers[i], nullptr);
        vkFreeMemory(device, mComputeUniformBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, mShaderStorageBuffers[i], nullptr);
        vkFreeMemory(device, mShaderStorageBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
    vkDestroyDescriptorPool(device, mImguiDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, mComputeDescriptorSetLayout, nullptr);
    vkDestroyShaderModule(device, mFragShaderModule, nullptr);
    vkDestroyShaderModule(device, mVertShaderModule, nullptr);
    vkDestroyShaderModule(device, mComputeShaderModule, nullptr);
    vkDestroyCommandPool(device, mCommandPool, nullptr);
    vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, mComputePipelineLayout, nullptr);
    vkDestroyPipeline(device, mComputePipeline, nullptr);
    vkDestroyPipeline(device, mGraphicsPipeline, nullptr);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(device, mComputeFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, mRenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, mInFlightFence[i], nullptr);
        vkDestroyFence(device, mComputeInFlightFences[i], nullptr);
    }
}

void VulkanRenderer::updateComputeUniformBuffer(U32 currentFrame, float dt)
{
    *static_cast<ComputeUniformBuffer*>(mComputeUniformBuffersMapped[currentFrame]) = {dt * 400};
}

void VulkanRenderer::updateUniformBuffer(U32 currentFrame, float dt, float modelAngle)
{
    /*static float secondsAccumulator{};
    secondsAccumulator += dt;*/

    MVPMatrices matrices
    {
        .model = glm::rotate(glm::mat4(1.0), modelAngle, glm::vec3(0, 0, 1.0)),

        .view = glm::lookAt(glm::vec3(1.7f, 1.7f, 1.7f),
            glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),

        .proj = glm::perspective(glm::radians(45.0f),
            mSwapChainExtent.width / (float)mSwapChainExtent.height, 0.1f, 10.0f)
    };

    matrices.proj[1][1] *= -1;

    std::memcpy(mUniformBuffersMapped[currentFrame], &matrices, sizeof matrices);
}

void VulkanRenderer::recordComputeCommands(VkCommandBuffer computeCmdBuff)
{
    VkCommandBufferBeginInfo const beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if(auto res{vkBeginCommandBuffer(computeCmdBuff, &beginInfo)}; res != VK_SUCCESS)
        throw DFException{"failed to begin recording command buffer!", res};

    vkCmdBindPipeline(computeCmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline);
    vkCmdBindDescriptorSets(computeCmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE,
        mComputePipelineLayout, 0, 1, &mComputeDescriptorSets[mCurrentFrame], 0, nullptr);

    vkCmdDispatch(computeCmdBuff, Particle::PARTICLE_COUNT / 256, 1, 1);

    if(auto res{vkEndCommandBuffer(computeCmdBuff)}; res != VK_SUCCESS)
        throw DFException{"vkEndCommandBuffer failed", res};
}

void VulkanRenderer::recordCommands(VkCommandBuffer cmdBuffer, VkCommandBuffer imguiCommandBuffer,
    U32 imageIndex, F32 deltaTime, glm::vec<2, double> mousePos)
{
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if(auto res{vkBeginCommandBuffer(cmdBuffer, &beginInfo)}; res != VK_SUCCESS)
        throw DFException{"vkBeginCommandBuffer failed", res};

    {//begin the main render pass

        std::array<VkClearValue, 2> clearColors {};
        clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearColors[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mRenderPass,
            .framebuffer = mSwapChainFramebuffers[imageIndex],
            .renderArea = { .extent = mSwapChainExtent },
            .clearValueCount = (U32)clearColors.size(),
            .pClearValues = clearColors.data()
        };

        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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
    
    {//bind the graphics pipeline, vertex buff, index buff, and descriptors

        //VkBuffer vertexBuffers[] = {mVertexBuff};
        VkDeviceSize offsets[] = {0};
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mShaderStorageBuffers[mCurrentFrame], offsets);
        //vkCmdBindIndexBuffer(cmdBuffer, mIndexBuff, 0, VK_INDEX_TYPE_UINT32);
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

    //vkCmdDrawIndexed(cmdBuffer, mIndices.size(), 1, 0, 0, 0);
    vkCmdDraw(cmdBuffer, Particle::PARTICLE_COUNT, 1, 0, 0);

    vkCmdEndRenderPass(cmdBuffer);

    {//record imgui commands in a different render pass

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(imguiCommandBuffer, &beginInfo);

        VkRenderPassBeginInfo const renderPassBeginInfo
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mImguiRenderPass,
            .framebuffer = mImguiFrameBuffers[imageIndex],
            .renderArea = { .extent = mSwapChainExtent },
        };

        vkCmdBeginRenderPass(imguiCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        ImDrawData* imguiDrawData {ImGui::GetDrawData()};
        ImGui_ImplVulkan_RenderDrawData(imguiDrawData, imguiCommandBuffer);
        vkCmdEndRenderPass(imguiCommandBuffer);

        vkEndCommandBuffer(imguiCommandBuffer);
    }

    if(auto res{vkEndCommandBuffer(cmdBuffer)}; res != VK_SUCCESS)
        throw DFException{"vkEndCommandBuffer failed", res};
}

void VulkanRenderer::update(F64 deltaTime, glm::vec<2, double> mousePos, float modelAngle)
{
    auto const device {mDevice.getLogicalDevice()};

    //updateUniformBuffer(mCurrentFrame, (float)deltaTime, modelAngle);

    {//submit to the compute queue

        vkWaitForFences(device, 1, &mComputeInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
        updateComputeUniformBuffer(mCurrentFrame, deltaTime);
        vkResetFences(device, 1, &mComputeInFlightFences[mCurrentFrame]);
        vkResetCommandBuffer(mComputeCommandBuffs[mCurrentFrame], 0);
        recordComputeCommands(mComputeCommandBuffs[mCurrentFrame]);

        VkSubmitInfo const submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &mComputeCommandBuffs[mCurrentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &mComputeFinishedSemaphores[mCurrentFrame]
        };

        if(auto res{vkQueueSubmit(mDevice.getComputeQueue(), 1, &submitInfo,
            mComputeInFlightFences[mCurrentFrame])}; res != VK_SUCCESS)
        {
            throw DFException{"vkQueueSubmit failed", res};
        };
    }

    //wait on mInFlightFence[mCurrentFrame] to be signaled, which indicates that the graphics queue
    //is done with mCommandBuffers[mCurrentFrame], and mCommandBuffers[mCurrentFrame] can now be recorded to again.
    vkWaitForFences(device, 1, &mInFlightFence[mCurrentFrame], VK_TRUE, UINT64_MAX);

    U32 imageIndex{0};
    {
        auto const res {vkAcquireNextImageKHR(device, mSwapChain, 
            UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex)};

        if(res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw DFException{"vkAcquireNextImageKHR failed", res};

        if(res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
    }

    vkResetFences(device, 1, &mInFlightFence[mCurrentFrame]);

    vkResetCommandBuffer(mCommandBuffs[mCurrentFrame], 0);
    recordCommands(mCommandBuffs[mCurrentFrame], mImguiCommandBuffs[mCurrentFrame], 
        imageIndex, deltaTime, mousePos);

    {//submit to the graphics queue

        auto const graphicsQueue {mDevice.getGraphicsQueue()};

        std::array const waitSemaphores
        {
            mComputeFinishedSemaphores[mCurrentFrame], mImageAvailableSemaphores[mCurrentFrame]
        };
        std::array<VkPipelineStageFlags, 2> const waitStages
        {
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        std::array<VkCommandBuffer, 2> const submitCommandBuffers
        {
            mCommandBuffs[mCurrentFrame],
            mImguiCommandBuffs[mCurrentFrame]
        };

        VkSubmitInfo const submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = (U32)waitSemaphores.size(),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = (U32)submitCommandBuffers.size(),
            .pCommandBuffers = submitCommandBuffers.data(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &mRenderFinishedSemaphores[mCurrentFrame],
        };

        //submit work to the graphics queue, and once that work is complete singal mInFlightFence[mCurrentFrame]
        if(auto res{vkQueueSubmit(graphicsQueue, 1, &submitInfo, mInFlightFence[mCurrentFrame])};
            res != VK_SUCCESS)
        {
            throw DFException{"vkQueueSubmit failed", res};
        }
    }

    {//submit to the present queue

        VkPresentInfoKHR presentInfo
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrame],
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

void VulkanRenderer::createImage(U32 width, U32 height, U32 mipLevels, VkFormat format, 
    VkSampleCountFlagBits numSamples,VkImageTiling tiling, VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
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
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = numSamples,
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
    VkImageLayout oldLayout, VkImageLayout newLayout, U32 mipLevels)
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
            .levelCount = mipLevels,
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

void VulkanRenderer::genMipMaps(VkImage img, VkFormat imgFmt, U32 width, U32 height, U32 mipLevels)
{
    //check if image format supports linear blitting
    VkFormatProperties fmtProperties{};
    vkGetPhysicalDeviceFormatProperties(mDevice.getPhysicalDevice(), imgFmt, &fmtProperties);
    if( ! (fmtProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) )
    {
        throw SystemInitException{"linear blitting not supported with vkCmdBlitImage"};
    }

    VkCommandBuffer cmdBuff = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = img,
        .subresourceRange =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    S32 mipWidth = width;
    S32 mipHeight = height;

    for(U32 i = 1; i < mipLevels; ++i)
    {
        //transfer the source mip level to transfer source optimal
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier
        (
            cmdBuff,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit imgBlit{};
        imgBlit.srcOffsets[0] = { 0, 0, 0 };
        imgBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        imgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgBlit.srcSubresource.mipLevel = i - 1;
        imgBlit.srcSubresource.baseArrayLayer = 0;
        imgBlit.srcSubresource.layerCount = 1;
        imgBlit.dstOffsets[0] = { 0, 0, 0 };
        imgBlit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        imgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgBlit.dstSubresource.mipLevel = i;
        imgBlit.dstSubresource.baseArrayLayer = 0;
        imgBlit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmdBuff, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgBlit, VK_FILTER_LINEAR);

        //now that this mip level has been copied from,
        //transition it to shader read only optimal
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier
        (
            cmdBuff,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }

    //transition the final mip map layer to shader read only optimal
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier
    (
        cmdBuff,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(cmdBuff);
}

void VulkanRenderer::initTextureImage()
{
    int texWidth{}, texHeight{}, texChannels{};

    stbi_uc* pixels {stbi_load(sTextureFpath.data(), &texWidth,
        &texHeight, &texChannels, STBI_rgb_alpha)};

    if ( ! pixels )
        throw SystemInitException{"failed to load image with stb_image"};

    mMipMapLevels = static_cast<uint32_t>(std::floor(
        std::log2(std::max(texWidth, texHeight)))) + 1;

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
    
    createImage(texWidth, texHeight, mMipMapLevels, FORMAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mTextureImage, mTextureImageMemory);

    transitionImageLayout(mTextureImage, FORMAT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipMapLevels);

    copyBufferToImage(stagingBuff, mTextureImage, (U32)texWidth, (U32)texHeight);

    genMipMaps(mTextureImage, FORMAT, texWidth, texHeight, mMipMapLevels);

    vkDestroyBuffer(device, stagingBuff, nullptr);
    vkFreeMemory(device, stagingBuffMemory, nullptr);
    stbi_image_free(pixels);
}

void VulkanRenderer::initTextureImageView()
{
    mTextureImageView = createImageView(mTextureImage, 
        FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, mMipMapLevels);
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
    samplerInfo.maxLod = (float)mMipMapLevels;

    auto device {mDevice.getLogicalDevice()};
    if(auto res{vkCreateSampler(device, &samplerInfo, nullptr, &mTextureSampler)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateSampler failed", res};
    }
}

void VulkanRenderer::initComputeDescriptorSets()
{
    auto device {mDevice.getLogicalDevice()};

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{};
    layouts.fill(mComputeDescriptorSetLayout);

    VkDescriptorSetAllocateInfo descSetAllocInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = (U32)MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts.data()
    };

    if(auto res {vkAllocateDescriptorSets(device, &descSetAllocInfo, mComputeDescriptorSets.data())};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateDescriptorSets failed", res};
    }

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo computeUniformBuffinfo
        {
            .buffer = mComputeUniformBuffers[i],
            .offset = 0,
            .range = sizeof ComputeUniformBuffer
        };

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = mComputeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo = &computeUniformBuffinfo;
        
        VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        storageBufferInfoLastFrame.buffer = mShaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = sizeof Particle * Particle::PARTICLE_COUNT;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = mComputeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = mShaderStorageBuffers[i];
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = sizeof Particle * Particle::PARTICLE_COUNT;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = mComputeDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), 
            descriptorWrites.data(), 0, nullptr);
    }
}

void VulkanRenderer::initImguiRenderPass()
{
    auto device {mDevice.getLogicalDevice()};

    VkAttachmentDescription const colorAttachment
    {
        .format = mSwapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    
    VkAttachmentReference const colorAttachmentRef
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    
    VkSubpassDescription const subpassDescription
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };
    
    VkSubpassDependency const subpassDependency
    {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };
    
    VkRenderPassCreateInfo const renderPassCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency
    };
    
    if(auto res {vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &mImguiRenderPass)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{
            "vkCreateRenderPass failed to create the imgui render pass", res};
    }
}

void VulkanRenderer::initImguiFrameBuffers()
{
    auto device {mDevice.getLogicalDevice()};

    VkFramebufferCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mImguiRenderPass,
        .attachmentCount = 1,
        .width = mSwapChainExtent.width,
        .height = mSwapChainExtent.height,
        .layers = 1
    };

    mImguiFrameBuffers.resize(mSwapChainImageViews.size());

    for(int i = 0; i < mSwapChainImageViews.size(); ++i)
    {
        info.pAttachments = &mSwapChainImageViews[i];

        if(auto res{vkCreateFramebuffer(device, &info, nullptr, &mImguiFrameBuffers[i])}; 
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateFramebuffer failed", res};
        }
    }
}

void VulkanRenderer::initImguiBackend()
{
    auto device {mDevice.getLogicalDevice()};

    {
        std::array<VkDescriptorPoolSize, 1> poolSizes
        {{
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1
            }
        }};

        VkDescriptorPoolCreateInfo poolCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1,
            .poolSizeCount = poolSizes.size(),
            .pPoolSizes = poolSizes.data()
        };

        auto const res {vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &mImguiDescriptorPool)};
        if(res != VK_SUCCESS)
            throw SystemInitException{"vkCreateDescriptorPool failed to make the imgui desc pool", res};
    }

    initImguiRenderPass();
    initImguiFrameBuffers();

    //pass vulkan info to imgui backend
    mImguiInitInfo.Instance = mDevice.getInstance();
    mImguiInitInfo.PhysicalDevice = mDevice.getPhysicalDevice();
    mImguiInitInfo.Device = mDevice.getLogicalDevice();
    mImguiInitInfo.QueueFamily = *mDevice.getQueueFamilyIndices().graphicsFamIdx;
    mImguiInitInfo.Queue = mDevice.getGraphicsQueue();
    mImguiInitInfo.PipelineCache = nullptr;
    mImguiInitInfo.DescriptorPool = mImguiDescriptorPool;
    mImguiInitInfo.RenderPass = mImguiRenderPass;
    mImguiInitInfo.Subpass = 0;
    mImguiInitInfo.MinImageCount = 2;
    mImguiInitInfo.ImageCount = mSwapChainImages.size();
    mImguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    mImguiInitInfo.Allocator = nullptr;
    mImguiInitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&mImguiInitInfo);

    mImGuiRenderInfoInitialized = true;
}

void VulkanRenderer::initSwapChainImageViews()
{
    mSwapChainImageViews.resize(mSwapChainImages.size());
    for(int i = 0; VkImage img : mSwapChainImages)
    {
        mSwapChainImageViews[i++] = createImageView(mSwapChainImages[i],
            mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void VulkanRenderer::initRenderPass()
{
    auto const device {mDevice.getLogicalDevice()};

    VkAttachmentDescription const colorAttachment
    {
        .format = mSwapChainImageFormat,
        .samples = mMSAASampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const resolveAttachment
    {
        .format = mSwapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,

         //imgui still needs to do its render pass after this, so this is
         //color attachment optimal layout instead of the present source layout
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const depthAttachment
    {
        .format = findDepthFormat(),
        .samples = mMSAASampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference const colorAttachmentRef
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
  
    VkAttachmentReference const depthAttachmentRef
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference const resolveAttachmentRef
    {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription const subpass
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &resolveAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };
    
    VkSubpassDependency const subpassDependency
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

    std::array const attachmentDescriptions {colorAttachment, depthAttachment, resolveAttachment};

    VkRenderPassCreateInfo const renderPassInfo
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
        if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mImageAvailableSemaphores[i])}; 
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateSemaphore failed", res};
        }

        if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mRenderFinishedSemaphores[i])};
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateSemaphore failed", res};
        }

        if(auto res{vkCreateFence(device, &fenceInfo, nullptr, &mInFlightFence[i])}; 
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateFence failed", res}; 
        }

        if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mComputeFinishedSemaphores[i])};
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateSemaphore failed", res};
        }

        if(auto res{vkCreateFence(device, &fenceInfo, nullptr, &mComputeInFlightFences[i])};
            res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateFence failed", res};
        }
    }
}

void VulkanRenderer::initComputeDescriptorSetLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings
    {{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        }
    }};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 3,
        .pBindings = layoutBindings.data()
    };

    auto device {mDevice.getLogicalDevice()};
    if(auto res {vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mComputeDescriptorSetLayout)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to create compute descriptor set layout!"};
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

    std::array const layoutBindings 
    {
        combinedImgSamplerLayoutBinding, 
        uniformBuffLayoutBinding,
    };

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

void VulkanRenderer::initComputeUniformBuffers()
{
    VkDeviceSize const buffSize {sizeof ComputeUniformBuffer};
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(buffSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mComputeUniformBuffers[i], mComputeUniformBuffersMemory[i]);

        //The UBO is mapped for the lifetime of the renderer.
        auto device {mDevice.getLogicalDevice()};
        vkMapMemory(device, mComputeUniformBuffersMemory[i], 0, buffSize, 0, &mComputeUniformBuffersMapped[i]);
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
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{};
    layouts.fill(mDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = (U32)MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts.data()
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
    std::array<VkDescriptorPoolSize, 3> const poolSizes
    {{
        {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            (U32)MAX_FRAMES_IN_FLIGHT * 2,
        },
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            (U32)MAX_FRAMES_IN_FLIGHT,
        },
        {
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            (U32)MAX_FRAMES_IN_FLIGHT * 2
        }
    }};

    VkDescriptorPoolCreateInfo poolInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = (U32)MAX_FRAMES_IN_FLIGHT * 2,
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

void VulkanRenderer::initComputeCommandBuffers()
{
    VkCommandBufferAllocateInfo const commandBuffAllocInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (U32)mComputeCommandBuffs.size()
    };

    if(auto res{vkAllocateCommandBuffers(mDevice.getLogicalDevice(), 
        &commandBuffAllocInfo, mComputeCommandBuffs.data())}; res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateCommandBuffers failed", res};
    }
}

void VulkanRenderer::initCommandBuffers()
{
    VkCommandBufferAllocateInfo const commandBuffAllocInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,

        //allocate MAX_FRAMES_IN_FLIGHT command buffers
        .commandBufferCount = (U32)mCommandBuffs.size()
    };

    if(auto res{vkAllocateCommandBuffers(mDevice.getLogicalDevice(), 
        &commandBuffAllocInfo, mCommandBuffs.data())}; res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateCommandBuffers failed", res};
    }
}

void VulkanRenderer::initImguiCommandBuffers()
{
    VkCommandBufferAllocateInfo const commandBuffAllocInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,

        //allocate MAX_FRAMES_IN_FLIGHT command buffers
        .commandBufferCount = (U32)mImguiCommandBuffs.size()
    };

    if(auto res{vkAllocateCommandBuffers(mDevice.getLogicalDevice(), 
        &commandBuffAllocInfo, mImguiCommandBuffs.data())}; res != VK_SUCCESS)
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
        .queueFamilyIndex = *queueFamilyIndices.graphicsFamIdx,
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

    vkDestroyImageView(device, mColorImageView, nullptr);
    vkDestroyImage(device, mColorImage, nullptr);
    vkFreeMemory(device, mColorImageMemory, nullptr);

    vkDestroyImageView(device, mDepthImageView, nullptr);
    vkDestroyImage(device, mDepthImage, nullptr);
    vkFreeMemory(device, mDepthImageMemory, nullptr);

    for(auto framebuffer : mSwapChainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    for(auto framebuffer : mImguiFrameBuffers)
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

    auto device {mDevice.getLogicalDevice()};

    vkDeviceWaitIdle(device);

    initSwapChain();
    initSwapChainImageViews();
    initColorResources();
    initDepthRescources();
    initFramebuffers();
}

void VulkanRenderer::initFramebuffers()
{
    mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

    for(int i = 0; i < mSwapChainImageViews.size(); ++i)
    {
        std::array const attachments
        {
            mColorImageView,
            mDepthImageView,
            mSwapChainImageViews[i]
        };

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

    for(auto const& format : availableFormats)
    {
        if(format.format == FORMAT && 
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
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
    U32 numOfSwapChainImages { swapChainSupportInfo.capabilities.minImageCount + 1 };

    //an image count of 0 inside the swap chain support info means that there is
    //no maximum amount of images we can have in our swap chain
    if(swapChainSupportInfo.capabilities.maxImageCount != 0 &&
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
        .oldSwapchain = mSwapChain
    };

    VulkanDevice::QueueFamilyIndices indices {mDevice.getQueueFamilyIndices()};
    U32 const queueFamilyIndices[2] {*indices.graphicsFamIdx, *indices.presentFamIdx};

    if(*indices.graphicsFamIdx != *indices.presentFamIdx)
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

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format,
    VkImageAspectFlags aspectFlags, U32 mipLevels)
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
            .levelCount = mipLevels,
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

void VulkanRenderer::initColorResources()
{
    VkFormat const colorFormat {mSwapChainImageFormat};
    VkImageUsageFlags const imageUsage
    {
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    createImage(mSwapChainExtent.width, mSwapChainExtent.height, 1, FORMAT, mMSAASampleCount, 
        VK_IMAGE_TILING_OPTIMAL, imageUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mColorImage, mColorImageMemory);

    mColorImageView = createImageView(mColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanRenderer::initDepthRescources()
{
    VkFormat const depthFormat {findDepthFormat()};

    createImage(mSwapChainExtent.width, mSwapChainExtent.height, 1, depthFormat, 
        mMSAASampleCount, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);

    mDepthImageView = createImageView(mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
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
        "resources/shaders/particles.vert", shaderc_vertex_shader);

    if(mVertShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    mFragShaderModule = compileGLSLShaders(device, 
        "resources/shaders/particles.frag", shaderc_fragment_shader);

    if(mFragShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    mComputeShaderModule = compileGLSLShaders(device,
        "resources/shaders/compute.spv", shaderc_compute_shader);

    if(mComputeShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    const auto end = std::chrono::steady_clock::now();

    Logger::get().fmtStdoutWarn("compiling glsl to Spir-V took {} seconds",
        std::chrono::duration<double, std::chrono::seconds::period>(end - start).count());
}

void VulkanRenderer::initShaderStorageBuffers()
{
    auto device {mDevice.getLogicalDevice()};

    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
    std::vector<Particle> particles;
    particles.reserve(Particle::PARTICLE_COUNT);

    size_t const height {20}, width {20};

    //init particle positions on a circle
    for(int i = 0; i < Particle::PARTICLE_COUNT; ++i)
    {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
        float x = r * cos(theta) * height / width;
        float y = r * sin(theta);
        
        particles.emplace_back
        (
            glm::vec2(x, y), //position
            glm::normalize(glm::vec2(x,y)) * 0.00025f, //velocity
            glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f) //color
        );
    }

    VkDeviceSize const buffSize {sizeof Particle * Particle::PARTICLE_COUNT};
    VkBuffer stagingBuff {VK_NULL_HANDLE};
    VkDeviceMemory stagingBuffMemory {VK_NULL_HANDLE};

    createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuff, stagingBuffMemory);

    void* data {nullptr};
    vkMapMemory(device, stagingBuffMemory, 0, buffSize, 0, &data);
    memcpy(data, particles.data(), (size_t)buffSize);
    vkUnmapMemory(device, stagingBuffMemory);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(buffSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mShaderStorageBuffers[i], 
            mShaderStorageBuffersMemory[i]);

        //copy from the staging buffer to the shader storage buffer
        copyBuffer(stagingBuff, mShaderStorageBuffers[i], buffSize);
    }

    vkDestroyBuffer(device, stagingBuff, nullptr);
    vkFreeMemory(device, stagingBuffMemory, nullptr);
}

void VulkanRenderer::initPipelineAndLayout()
{
    std::array<VkPipelineShaderStageCreateInfo, 2> graphicsShaderStages
    {{
        {//vertex
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = mVertShaderModule,
            .pName = "main"
        },
        {//fragment
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = mFragShaderModule,
            .pName = "main"
        }
    }};

    VkVertexInputBindingDescription const bindingDescription{
        Particle::getBindingDescription()};

    auto const attributeDescriptions {Particle::getAttributeDescriptions()};

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
        //.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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
        .rasterizationSamples = mMSAASampleCount,
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

    VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = graphicsShaderStages.size(),
        .pStages = graphicsShaderStages.data(),
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
        &graphicsPipelineCreateInfo, nullptr, &mGraphicsPipeline)}; res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to create graphics pipeline!"};
    }

    VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mComputeDescriptorSetLayout
    };

    vkCreatePipelineLayout(device, &computePipelineLayoutCreateInfo, 
        nullptr, &mComputePipelineLayout);

    VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = mComputeShaderModule,
        .pName = "main"
    };

    VkComputePipelineCreateInfo computePipelineCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageCreateInfo,
        .layout = mComputePipelineLayout
    };

    if(auto res{vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
        &computePipelineCreateInfo, nullptr, &mComputePipeline)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to create compute pipeline!", res};
    }
}

}//end namespace DF