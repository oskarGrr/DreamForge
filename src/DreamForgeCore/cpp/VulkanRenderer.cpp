#include "VulkanRenderer.hpp"
#include "errorHandling.hpp"
#include "Logging.hpp"
#include "shaderc/shaderc.hpp"

#include <filesystem>
#include <fstream>
#include <span>

namespace DF
{

VulkanRenderer::VulkanRenderer(Window const& wnd) : mWindow{wnd}, mDevice{wnd}
{  
    createSwapChain();
    createImageViews();
    createRenderPass();
    createPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSynchronizationObjects();
}

VulkanRenderer::~VulkanRenderer()
{
    auto device {mDevice.getLogicalDevice()};

    vkDeviceWaitIdle(device);

    for(auto imageView : mSwapChainImageViews)
        vkDestroyImageView(device, imageView, nullptr);

    for(auto framebuffer : mSwapChainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    vkDestroySwapchainKHR(device, mSwapChain, nullptr);
    vkDestroyRenderPass(device, mRenderPass, nullptr);
    vkDestroyShaderModule(device, mFragShaderModule, nullptr);
    vkDestroyShaderModule(device, mVertShaderModule, nullptr);
    vkDestroyCommandPool(device, mCommandPool, nullptr);
    vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    vkDestroyPipeline(device, mGraphicsPipeline, nullptr);

    vkDestroySemaphore(mDevice.getLogicalDevice(), mImageAvailableSem, nullptr);
    vkDestroySemaphore(mDevice.getLogicalDevice(), mRenderFinishedSem, nullptr);
    vkDestroyFence(mDevice.getLogicalDevice(), mInFlightFence, nullptr);
}

void VulkanRenderer::createSynchronizationObjects()
{
    VkSemaphoreCreateInfo const semCreateInfo {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    auto const device {mDevice.getLogicalDevice()};

    if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mImageAvailableSem)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateSemaphore failed", res};
    }

    if(auto res{vkCreateSemaphore(device, &semCreateInfo, nullptr, &mRenderFinishedSem)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateSemaphore failed", res};
    }

    VkFenceCreateInfo const fenceInfo 
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,

        //start the fence in the signaled state so the first call to update() does not block indefinitely
        .flags = VK_FENCE_CREATE_SIGNALED_BIT 
    };

    if(auto res{vkCreateFence(device, &fenceInfo, nullptr, &mInFlightFence)}; res != VK_SUCCESS)
        throw SystemInitException{"vkCreateFence failed", res};
}

void VulkanRenderer::createImageViews()
{
    mSwapChainImageViews.resize(mSwapChainImages.size());

    for(size_t i = 0; VkImage img : mSwapChainImages)
    {
        VkImageViewCreateInfo createInfo
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = mSwapChainImageFormat,
        };

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        //createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if(auto res{vkCreateImageView(mDevice.getLogicalDevice(), &createInfo, nullptr, 
            &mSwapChainImageViews[i])}; res != VK_SUCCESS)
        {
            throw SystemInitException{"vkCreateFence failed", res};
        }

        ++i;
    }
}

void VulkanRenderer::createRenderPass()
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

    VkAttachmentReference colorAttachmentRef
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
  
    VkSubpassDescription subpass
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };
    
    VkSubpassDependency subpassDependency
    {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassInfo
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,  
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency
    };

    if(auto res{vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateRenderPass failed "};
    }  
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        //.flags = 0,
        //.pInheritanceInfo = nullptr
    };

    if(auto res{vkBeginCommandBuffer(commandBuffer, &beginInfo)}; res != VK_SUCCESS)
        throw DFException{"vkBeginCommandBuffer failed", res};
    
    VkRenderPassBeginInfo renderPassInfo
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mRenderPass,
        .framebuffer = mSwapChainFramebuffers[imageIndex]
    };

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = mSwapChainExtent;

    VkClearValue clearColor {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    VkViewport const viewport
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(mSwapChainExtent.width),
        .height = static_cast<float>(mSwapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D const scissor
    {
        .offset = {0, 0},
        .extent = mSwapChainExtent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if(auto res{vkEndCommandBuffer(commandBuffer)}; res != VK_SUCCESS)
        throw DFException{"vkEndCommandBuffer failed", res};
}

void VulkanRenderer::update()
{
    auto const device {mDevice.getLogicalDevice()};
    auto const graphicsQueue {mDevice.getGraphicsQueue()};

    vkWaitForFences(device, 1, &mInFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &mInFlightFence);

    U32 imageIndex{0};
    vkAcquireNextImageKHR(device, mSwapChain, 
        UINT64_MAX, mImageAvailableSem, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(mCommandBuffer, 0);
    recordCommandBuffer(mCommandBuffer, imageIndex);

    VkPipelineStageFlags const waitStage {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mImageAvailableSem,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &mCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mRenderFinishedSem,
    };

    if(auto res{vkQueueSubmit(graphicsQueue, 1, &submitInfo, mInFlightFence)}; res != VK_SUCCESS)
        throw DFException{"vkQueueSubmit failed", res};

    VkPresentInfoKHR presentInfo
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mRenderFinishedSem,
        .swapchainCount = 1,
        .pSwapchains = &mSwapChain,
        .pImageIndices = &imageIndex,
        //.pResults = nullptr
    };

    vkQueuePresentKHR(mDevice.getPresentQueue(), &presentInfo);
}

void VulkanRenderer::createCommandBuffer()
{
    VkCommandBufferAllocateInfo const commandBuffAllocInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if(auto res{vkAllocateCommandBuffers(mDevice.getLogicalDevice(), 
        &commandBuffAllocInfo, &mCommandBuffer)}; res != VK_SUCCESS)
    {
        throw SystemInitException{"vkAllocateCommandBuffers failed", res};
    }
}

void VulkanRenderer::createCommandPool()
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

void VulkanRenderer::createFramebuffers()
{
    mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

    for(size_t i = 0; i < mSwapChainImageViews.size(); i++) 
    {
        std::array<VkImageView, 1> attachments {mSwapChainImageViews[i]};

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

    return VK_PRESENT_MODE_FIFO_KHR;
}

//helper func for createSwapChain
[[nodiscard]]
static VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(std::span<const VkSurfaceFormatKHR> availableFormats)
{
    assert( ! availableFormats.empty() );

    for(auto const& availableFormat : availableFormats) 
    {
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

void VulkanRenderer::createSwapChain()
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
        .oldSwapchain = VK_NULL_HANDLE,
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

[[nodiscard]]
static VkShaderModule createShaderModule(U32 const* const spirVCode, size_t spirVSize, VkDevice device)
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
static VkShaderModule compileGLSLShaders(VkDevice device, std::string_view shaderPath, shaderc_shader_kind shaderType)
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

    return createShaderModule(compilationResult.cbegin(), spirVlen, device);
}

void VulkanRenderer::createPipeline()
{
    auto const device {mDevice.getLogicalDevice()};

    Logger::get().stdoutInfo("Compiling shaders...");

    //TODO directory iterator and compile anything that has the right extension and also check time stamps
    mVertShaderModule = compileGLSLShaders(device, 
        "resources/shaders/triangleTest.vert", shaderc_vertex_shader);

    if(mVertShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    mFragShaderModule = compileGLSLShaders(device, 
        "resources/shaders/triangleTest.frag", shaderc_fragment_shader);

    if(mFragShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

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

    //no vertex data for now (just testing)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        //.vertexBindingDescriptionCount = 0,
        //.pVertexBindingDescriptions = nullptr,
        //.vertexAttributeDescriptionCount = 0,
        //.pVertexAttributeDescriptions = nullptr
    };

    std::vector<VkDynamicState> const dynamicStates {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
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
        .width = static_cast<float>(mSwapChainExtent.width),
        .height = static_cast<float>(mSwapChainExtent.height),
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
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
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

    //colorBlendAttachment.blendEnable = VK_TRUE;
    //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    VkPipelineColorBlendStateCreateInfo colorBlending
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        //.logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    //colorBlending.blendConstants[0] = 0.0f,
    //colorBlending.blendConstants[1] = 0.0f,
    //colorBlending.blendConstants[2] = 0.0f,
    //colorBlending.blendConstants[3] = 0.0f

    VkPipelineLayoutCreateInfo const pipelineLayoutInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    if(auto res{vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vKCreatePipelineLayout failed ", res};
    }

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
        .pDepthStencilState = nullptr,
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