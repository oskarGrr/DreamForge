#include "VulkanDevice.hpp"
#include "ErrorHandling.hpp"
#include "HelpfulTypeAliases.hpp"
#include "Logging.hpp"

#include <GLFW/glfw3.h>

#include <span>
#include <cstddef>
#include <vector>
#include <unordered_set>
#include <algorithm> //std::clamp
#include <limits>

#ifdef DF_DEBUG
    #define USE_VALIDATION_LAYERS
#endif

namespace DF
{

static VKAPI_ATTR VkBool32 VKAPI_CALL 
debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT msgType, 
    VkDebugUtilsMessengerCallbackDataEXT const* vulkanData, void* userData)
{
    std::string msg { "vulkan validation layer:\n" };
    msg.append(vulkanData->pMessage);

    if(msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        Logger::get().stdoutInfo(msg);
    else if(msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        Logger::get().stdoutWarn(msg);
    else if(msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        Logger::get().stdoutError(msg);

    return VK_FALSE;
}

VulkanDevice::VulkanDevice(Window const& wnd) : mWindowRef{wnd}
{
    //get which glfw related vulkan instance extensions are needed
    uint32_t glfwExtensionCount {0};
    char const* const* const glfwExtensions {glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};
    std::vector<char const*> instanceExtensions {glfwExtensionCount};

    for(int i = 0; i < glfwExtensionCount; ++i)
        instanceExtensions[i] = glfwExtensions[i];

#ifdef USE_VALIDATION_LAYERS
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    createInstance(instanceExtensions); //fill in mInstance handle
    createSurface(); //fill in mSurface handle
    selectPhysicalDevice(); //fill in mPhysicalDevice handle
    createLogicalDevice();

#ifdef DF_DEBUG
    logSupportedInstanceExtensions();
#endif
}

VulkanDevice::~VulkanDevice()
{
#ifdef USE_VALIDATION_LAYERS

    auto destoryDebugMessenger {reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT> 
        (vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT"))};

    if(destoryDebugMessenger)
        destoryDebugMessenger(mInstance, mDebugMessenger, nullptr);

#endif

    vkDestroyDevice(mLogicalDevice, nullptr);
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyInstance(mInstance, nullptr);
}

VkSampleCountFlagBits VulkanDevice::getMaxUsableSampleCount() const
{
    auto const colorSampleCounts {mDeviceProperties.limits.framebufferColorSampleCounts};
    auto const depthSampleCounts {mDeviceProperties.limits.framebufferDepthSampleCounts};
    VkSampleCountFlags const sampleCountFlags {colorSampleCounts & depthSampleCounts};

    if(sampleCountFlags & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if(sampleCountFlags & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if(sampleCountFlags & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if(sampleCountFlags & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
    if(sampleCountFlags & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
    if(sampleCountFlags & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

//return the indices to the graphics and present queue families for mPhysicalDevice
VulkanDevice::QueueFamilyIndices VulkanDevice::getQueueFamilyIndicesImpl(VkPhysicalDevice physicalDeviceHandle) const
{
    QueueFamilyIndices retval {.graphicsFamilyIndex = std::nullopt, .presentFamilyIndex = std::nullopt};

    //first query for how many queue family properties there are for this physical device
    U32 numQueueFamProperties {};
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &numQueueFamProperties, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamProperties {numQueueFamProperties};
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &numQueueFamProperties, queueFamProperties.data());

    for(U32 i {0}; i < numQueueFamProperties; ++i)
    {
        if(queueFamProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            retval.graphicsFamilyIndex = i;

        VkBool32 queueHasPresentationSupport {VK_FALSE};
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeviceHandle, i, mSurface, &queueHasPresentationSupport);

        if(queueHasPresentationSupport)
            retval.presentFamilyIndex = i;
    }

    return retval;
}

void VulkanDevice::getSwapChainSupportInfoImpl(VkPhysicalDevice deviceHandle, SwapChainSupportDetails& outDetails) const
{
    if(auto res {vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceHandle, mSurface, &outDetails.capabilities)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed", res};
    }
    
    U32 formatCount {0};
    if(auto res {vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, mSurface, &formatCount, nullptr)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkGetPhysicalDeviceSurfaceFormatsKHR failed", res};
    }
    
    if(formatCount != 0)
    {
        outDetails.formats.resize(formatCount);
        auto res {vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, mSurface, &formatCount, 
            outDetails.formats.data())};

        if(res != VK_SUCCESS)
            throw SystemInitException{"vkGetPhysicalDeviceSurfaceFormatsKHR failed", res};
    }
    
    U32 numPresentModes {0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, mSurface, &numPresentModes, nullptr);
    
    if(numPresentModes != 0)
    {
        outDetails.presentModes.resize(numPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, mSurface, &numPresentModes, 
            outDetails.presentModes.data());
    }
}

bool VulkanDevice::isPhysicalDeviceSuitable(VkPhysicalDevice deviceHandle)
{
    //check if the device extensions are supported
    {
        //check if mPhysicalDevice supports the extensions we want (for now just the swap chain extension)
        U32 extensionCount {0};
        auto res {vkEnumerateDeviceExtensionProperties(deviceHandle, nullptr, &extensionCount, nullptr)};
        std::vector<VkExtensionProperties> availableExtensions {extensionCount};
        res = vkEnumerateDeviceExtensionProperties(deviceHandle, nullptr,
            &extensionCount, availableExtensions.data());
            
        if(res != VK_SUCCESS)
            throw SystemInitException{"vkEnumerateDeviceExtensionProperties failed", res};

        for(const char* const extension : mDeviceExtensions)
        {
            auto it {std::find_if(availableExtensions.begin(), availableExtensions.end(),
                [extension](auto const& availableExtension)
                { return strcmp(extension, availableExtension.extensionName) == 0; }
            )};
            
            //if this device extension is not in the list of available devices extensions
            if(it == availableExtensions.end())
                return false;
        }
    }

    //check if the swap chain information is suitable
    {
        SwapChainSupportDetails swapChainSupport;
        getSwapChainSupportInfoImpl(deviceHandle, swapChainSupport);
        
        if(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
            return false;
    }
    
    //check if the required queue families are supported
    {
        QueueFamilyIndices const queueFamIndices {getQueueFamilyIndicesImpl(deviceHandle)};
        if( ! queueFamIndices.graphicsFamilyIndex.has_value() || ! queueFamIndices.presentFamilyIndex.has_value() )
            return false;
    }

    //check if the device features we need are supported
    {
        //query for the supported features for this physical device
        VkPhysicalDeviceFeatures supportedDeviceFeatures {};
        vkGetPhysicalDeviceFeatures(deviceHandle, &supportedDeviceFeatures);

        //the list of required device features.
        //if any of them are false then this device is not suitable
        if(//supportedDeviceFeatures.tessellationShader != VK_TRUE ||
           //supportedDeviceFeatures.geometryShader     != VK_TRUE ||
           supportedDeviceFeatures.samplerAnisotropy    != VK_TRUE)
        {
            return false;
        }
        
        //at this point this deivce (deviceHandle) is suitable

        //enable the required features
        //mDeviceFeatures.tessellationShader = VK_TRUE;
        //mDeviceFeatures.geometryShader     = VK_TRUE;
        mDeviceFeatures.samplerAnisotropy  = VK_TRUE;

        //enable multi draw indirect only if its supported
        mDeviceFeatures.multiDrawIndirect = supportedDeviceFeatures.multiDrawIndirect;
    }

    return true;
}

//init mPhysicalDevice from mAllPhysicalDevices
void VulkanDevice::selectPhysicalDevice()
{
    //fill mAllPhysicalDevices
    enumerateAllPhysicalDevices();

    if(mAllPhysicalDevices.size() == 0)
        throw SystemInitException{"vulkan found no physical devices"};

    //find the first discrete gpu
    for(VkPhysicalDevice const deviceHandle : mAllPhysicalDevices)
    {
        if(isPhysicalDeviceSuitable(deviceHandle))
        {
            mPhysicalDevice = deviceHandle;
            break;
        }
    }

    if(mPhysicalDevice == VK_NULL_HANDLE)
        throw SystemInitException{"failed to find a suitable physical device"};

    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);

    mMaxMSAASampleCount = getMaxUsableSampleCount();

    std::string deviceFoundMsg;
    if(mDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        deviceFoundMsg = "using GPU: ";
        deviceFoundMsg.append(mDeviceProperties.deviceName);
        Logger::get().stdoutInfo(deviceFoundMsg); //just info
    }
    else //not a discrete GPU
    {
        deviceFoundMsg = "could not find a discrete GPU. Using physical device:\n";
        deviceFoundMsg.append(mDeviceProperties.deviceName);
        Logger::get().stdoutWarn(deviceFoundMsg); //warning
    }
}

void VulkanDevice::createSurface()
{
    mSurface = mWindowRef.createVulkanSurface(mInstance);
}

void VulkanDevice::logSupportedInstanceExtensions() const
{
    //get how many extensions are supported, and allocate enough room for all of their properties with a vector
    U32 numExtensions {0};
    vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> extensionPropertiesVec {numExtensions};

    Logger::get().fmtStdoutInfo("{} instance extensions supported", numExtensions);

    //get vulkan to fill the vector with the extension property data for each supported extension
    vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensionPropertiesVec.data());

    //log each extension name
    for(auto const& extention : extensionPropertiesVec)
    {
        Logger::get().stdoutInfo(extention.extensionName);
    }
}

//fill mAllPhysicalDevices
void VulkanDevice::enumerateAllPhysicalDevices()
{
    //figure out how many physical devices are on this system
    U32 numPhysicalDevices {0};
    if(VkResult res{vkEnumeratePhysicalDevices(mInstance, &numPhysicalDevices, nullptr)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkEnumeratePhysicalDevices failed", res};
    }

    if(numPhysicalDevices == 0)
    {
        throw SystemInitException{"can't find any physical devices compatible with vulkan"};
    }

    //make sure there is enough room in the vector for all the device handles
    mAllPhysicalDevices.resize(numPhysicalDevices);

    //enumerate again, this time filling mPhysicalDevices with numPhysicalDevices devices
    if(VkResult res{vkEnumeratePhysicalDevices(mInstance, &numPhysicalDevices, mAllPhysicalDevices.data())};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkEnumeratePhysicalDevices failed", res};
    }
}

//init mLogicalDevice from mPhysicalDevice
void VulkanDevice::createLogicalDevice()
{
    //these queue families are already checked in isPhysicalDeviceSuitable()
    QueueFamilyIndices const queueFamIndices { getQueueFamilyIndicesImpl(mPhysicalDevice) };
    
    std::unordered_set<U32> const uniqueQueueFamIndecies
    {
        *queueFamIndices.graphicsFamilyIndex, 
        *queueFamIndices.presentFamilyIndex
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreationInformation;
    queueCreationInformation.reserve(uniqueQueueFamIndecies.size());
    const float queuePriorities {1.0f};

    for(U32 const index : uniqueQueueFamIndecies)
    {
        queueCreationInformation.emplace_back
        (
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, //sType
            nullptr,                                    //pNext
            0,                                          //flags
            index,                                      //queueFamilyIndex
            1,                                          //queueCount
            &queuePriorities                            //pQueuePriorities
        );
    }

    VkDeviceCreateInfo deviceCreationInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<U32>(queueCreationInformation.size()), 
        .pQueueCreateInfos = queueCreationInformation.data(),
        .enabledExtensionCount = static_cast<U32>(mDeviceExtensions.size()),
        .ppEnabledExtensionNames = mDeviceExtensions.data(),
        .pEnabledFeatures = &mDeviceFeatures,
    };
    
    if(VkResult res {vkCreateDevice(mPhysicalDevice, &deviceCreationInfo, nullptr, &mLogicalDevice)}; 
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateDevice failed", res};
    }

    //get the handles to the 2 queues
    vkGetDeviceQueue(mLogicalDevice, *queueFamIndices.graphicsFamilyIndex, 0, &mGraphicsQueue);
    vkGetDeviceQueue(mLogicalDevice, *queueFamIndices.presentFamilyIndex, 0, &mPresentQueue);
}

void VulkanDevice::createInstance(std::vector<const char*>& extensions)
{
    VkApplicationInfo appCreationInfo 
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Dream Forge",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo instanceCreationInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appCreationInfo
    };

#ifdef USE_VALIDATION_LAYERS
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreationInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,

        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

        .pfnUserCallback = debugMessengerCallback
    };

    const char* layerName {"VK_LAYER_KHRONOS_validation"};
    instanceCreationInfo.enabledLayerCount = 1;
    instanceCreationInfo.ppEnabledLayerNames = &layerName;
    instanceCreationInfo.pNext = &debugMessengerCreationInfo;
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif //USE_VALIDATION_LAYERS

    instanceCreationInfo.enabledExtensionCount = extensions.size();
    instanceCreationInfo.ppEnabledExtensionNames = extensions.data();
    if(auto res{vkCreateInstance(&instanceCreationInfo, nullptr, &mInstance)}; res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateInstance failed", res};
    }

#ifdef USE_VALIDATION_LAYERS
    auto* createDebugUtilsMessengerEXT { reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>
        (vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT"))
    };

    if(auto res{createDebugUtilsMessengerEXT(mInstance, &debugMessengerCreationInfo, nullptr, &mDebugMessenger)};
        res != VK_SUCCESS)
    {
        throw SystemInitException{"vkCreateDebugUtilsMessengerEXT failed", res};
    }
#endif //USE_VALIDATION_LAYERS

}

}//end namespace DF