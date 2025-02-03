#include "pipeline.hpp"
#include "Logging.hpp"
#include "HelpfulTypeAliases.hpp"
#include "errorHandling.hpp"

#include <string>
#include <format>
#include <filesystem>
#include <fstream>

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.hpp>

namespace DF
{

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

static std::string getFileAsString(std::filesystem::path const& fname)
{
    std::ifstream ifs{fname, std::ios_base::ate | std::ios_base::binary};

    if( ! ifs )
    {
        throw DFException { std::string{"could not open "}.append(fname.string()) };
    }

    std::string retval;
    retval.resize(ifs.tellg());
    ifs.seekg(0);
    ifs.read(retval.data(), retval.size());

    return retval;
}

Pipeline::Pipeline(VkDevice logicalDevice, VkExtent2D swapChainExtent, VkRenderPass renderPass) 
    : mLogicalDevice{logicalDevice}
{
    Logger::get().stdoutInfo("Compiling shaders...");

    //TODO directory iterator and compile anything that has the right extension and also check time stamps
    mVertShaderModule = compileGLSLShaders("resources/shaders/triangleTest.vert", shaderc_vertex_shader);

    if(mVertShaderModule == VK_NULL_HANDLE)
        throw SystemInitException{"Problem creating shader modules/compiling shaders"};

    mFragShaderModule = compileGLSLShaders("resources/shaders/triangleTest.frag", shaderc_fragment_shader);

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
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D const scissor{ .offset = {0, 0}, .extent = swapChainExtent };

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

    if(auto res{vkCreatePipelineLayout(mLogicalDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout)};
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
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if(auto res{vkCreateGraphicsPipelines(mLogicalDevice, VK_NULL_HANDLE, 1,
        &pipelineInfo, nullptr, &mGraphicsPipeline)}; res != VK_SUCCESS)
    {
        throw SystemInitException{"failed to create graphics pipeline!"};
    }


}

Pipeline::~Pipeline()
{
    vkDestroyShaderModule(mLogicalDevice, mFragShaderModule, nullptr);
    vkDestroyShaderModule(mLogicalDevice, mVertShaderModule, nullptr);
    vkDestroyPipeline(mLogicalDevice, mGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mLogicalDevice, mPipelineLayout, nullptr);
}

[[nodiscard]] //returns VK_NULL_HANDLE if something went wrong
VkShaderModule Pipeline::compileGLSLShaders(std::string_view shaderPath, shaderc_shader_kind shaderType)
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

    return createShaderModule(compilationResult.cbegin(), spirVlen, mLogicalDevice);
}

}