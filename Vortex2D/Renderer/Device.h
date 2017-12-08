//
//  Device.h
//  Vortex2D
//

#ifndef Vortex2d_Device_h
#define Vortex2d_Device_h

#include <Vortex2D/Renderer/Common.h>
#include <Vortex2D/Renderer/DescriptorSet.h>
#include <Vortex2D/Utils/vk_mem_alloc.h>
#include <map>

namespace Vortex2D { namespace Renderer {

class Device
{
public:
    Device(vk::PhysicalDevice physicalDevice, bool validation = true);
    Device(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool validation = true);
    Device(vk::PhysicalDevice physicalDevice, int familyIndex, bool validation = true);
    ~Device();

    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    // Vulkan handles and helpers
    vk::Device Handle() const;
    vk::Queue Queue() const;
    vk::PhysicalDevice GetPhysicalDevice() const;
    int GetFamilyIndex() const;

    // Command buffer functions
    std::vector<vk::CommandBuffer> CreateCommandBuffers(uint32_t size) const;
    void FreeCommandBuffers(vk::ArrayProxy<const vk::CommandBuffer> commandBuffers) const;

    // Memory allocator
    VmaAllocator Allocator() const;
    LayoutManager& GetLayoutManager() const;


    vk::ShaderModule GetShaderModule(const std::string& filename) const;
    const std::vector<uint32_t> GetShaderSPIRV(const std::string& filename) const;

private:
    vk::PhysicalDevice mPhysicalDevice;
    int mFamilyIndex;
    vk::UniqueDevice mDevice;
    vk::Queue mQueue;
    vk::UniqueCommandPool mCommandPool;
    vk::UniqueDescriptorPool mDescriptorPool;
    VmaAllocator mAllocator;

    struct SPIRVModule
    {
      std::vector<uint32_t> Binary;
      vk::UniqueShaderModule Module;
    };

    mutable std::map<std::string, SPIRVModule> mShaders;
    mutable LayoutManager mLayoutManager;
};

}}

#endif
