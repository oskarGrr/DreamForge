#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

#include "Window.hpp"

namespace DF
{

//sets up vulkan related stuff
class VulkanDevice
{
public:

    VulkanDevice(Window const& wnd);
    ~VulkanDevice();

    auto getLogicalDevice() const {return mLogicalDevice;}
    auto getPhysicalDevice() const {return mPhysicalDevice;}
    auto getGraphicsQueue() const {return mGraphicsQueue;}
    auto getPresentQueue() const {return mPresentQueue;}
    auto getComputeQueue() const {return mComputeQueue;}
    auto getSurface() const {return mSurface;}
    auto getMaxMsaaSampleCount() const {return mMaxMSAASampleCount;}
    auto getInstance() const {return mInstance;}
    auto const& getPhysicalDeviceProperties() const {return mDeviceProperties;}

    struct QueueFamilyIndices
    {
        std::optional<U32> graphicsFamIdx{std::nullopt}, 
            presentFamIdx{std::nullopt}, graphicsAndComputeFamIdx{std::nullopt};
    };

    QueueFamilyIndices getQueueFamilyIndices() const
    {
        return getQueueFamilyIndicesImpl(mPhysicalDevice);
    }

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    void getSwapChainSupportInfo(SwapChainSupportDetails& outDetails) const
    {
        getSwapChainSupportInfoImpl(mPhysicalDevice, outDetails);
    }

    void logSupportedInstanceExtensions() const;

private:

    void createInstance(std::vector<const char*>& extensions); //init mInstance
    void enumerateAllPhysicalDevices(); //fill mAllPhysicalDevices
    void selectPhysicalDevice(); //init mPhysicalDevice from mAllPhysicalDevices
    void createSurface(); //init mSurface
    void getSwapChainSupportInfoImpl(VkPhysicalDevice, SwapChainSupportDetails& outDetails) const;
    void createLogicalDevice(); //init mLogicalDevice from mPhysicalDevice
    QueueFamilyIndices getQueueFamilyIndicesImpl(VkPhysicalDevice) const;
    VkSampleCountFlagBits getMaxUsableSampleCount() const;

    //If the resut holds a string there was an error or the physicalDevice is not suitable,
    //and the string has an explanation, ohterwise the physical device is suitable.
    bool isPhysicalDeviceSuitable(VkPhysicalDevice);

    //vulkan handles
    VkInstance mInstance {VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT mDebugMessenger{VK_NULL_HANDLE};
    VkSurfaceKHR mSurface {VK_NULL_HANDLE};
    VkDevice mLogicalDevice {VK_NULL_HANDLE};
    VkQueue mGraphicsQueue {VK_NULL_HANDLE};
    VkQueue mPresentQueue {VK_NULL_HANDLE};
    VkQueue mComputeQueue {VK_NULL_HANDLE};
    VkPhysicalDevice mPhysicalDevice {VK_NULL_HANDLE};
    std::vector<VkPhysicalDevice> mAllPhysicalDevices;
    VkSampleCountFlagBits mMaxMSAASampleCount {VK_SAMPLE_COUNT_1_BIT};

    //the validation layers and device extensions that need to be supported.
    //the device extensions will be checked for validity in isPhysicalDeviceSuitable().
    std::vector<const char*> const mValidationLayers {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> const mDeviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    //after selectPhysicalDevice has selected a suitable device,
    //this structure will hold the features we will be enabling in createLogicalDevice
    VkPhysicalDeviceFeatures mDeviceFeatures {};

    //filled out in selectPhysicalDevice
    VkPhysicalDeviceProperties mDeviceProperties {};

    Window const& mWindowRef;

public:
    VulkanDevice(VulkanDevice const&)=delete;
    VulkanDevice(VulkanDevice&&)=delete;
    VulkanDevice& operator=(VulkanDevice const&)=delete;
    VulkanDevice& operator=(VulkanDevice&&)=delete;
};

}