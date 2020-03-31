//
//  Texture.cpp
//  Vortex2D
//

#include <Vortex2D/Renderer/CommandBuffer.h>
#include <Vortex2D/Renderer/Device.h>
#include <Vortex2D/Renderer/Texture.h>

namespace Vortex2D
{
namespace Renderer
{
void TextureBarrier(vk::Image image,
                    vk::CommandBuffer commandBuffer,
                    vk::ImageLayout oldLayout,
                    vk::AccessFlags srcMask,
                    vk::ImageLayout newLayout,
                    vk::AccessFlags dstMask)
{
  auto imageMemoryBarriers = vk::ImageMemoryBarrier()
                                 .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                 .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                 .setOldLayout(oldLayout)
                                 .setNewLayout(newLayout)
                                 .setImage(image)
                                 .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                                 .setSrcAccessMask(srcMask)
                                 .setDstAccessMask(dstMask);

  commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eAllCommands | vk::PipelineStageFlagBits::eHost,
      vk::PipelineStageFlagBits::eAllCommands | vk::PipelineStageFlagBits::eHost,
      {},
      nullptr,
      nullptr,
      imageMemoryBarriers);
}

std::uint64_t GetBytesPerPixel(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eR8Uint:
    case vk::Format::eR8Sint:
      return 1;
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32Sint:
    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eB8G8R8A8Unorm:
      return 4;
    case vk::Format::eR32G32Sfloat:
      return 8;
    case vk::Format::eR32G32B32A32Sfloat:
      return 16;
    default:
      throw std::runtime_error("unsupported format");
  }
}

vk::SamplerAddressMode ConvertAddressMode(Sampler::AddressMode addressMode)
{
  switch (addressMode)
  {
    case Sampler::AddressMode::Repeat:
      return vk::SamplerAddressMode::eRepeat;
    case Sampler::AddressMode::ClampToEdge:
      return vk::SamplerAddressMode::eClampToEdge;
  }
}

vk::Filter ConvertFilter(Sampler::Filter filter)
{
  switch (filter)
  {
    case Sampler::Filter::Linear:
      return vk::Filter::eLinear;
    case Sampler::Filter::Nearest:
      return vk::Filter::eNearest;
  }
}

Sampler::Sampler(Device& device, AddressMode addressMode, Filter filter)
{
  auto samplerInfo = vk::SamplerCreateInfo()
                         .setMaxAnisotropy(1.0f)
                         .setMagFilter(ConvertFilter(filter))
                         .setMinFilter(ConvertFilter(filter))
                         .setAddressModeU(ConvertAddressMode(addressMode))
                         .setAddressModeV(ConvertAddressMode(addressMode));

  mSampler = device.Handle().createSamplerUnique(samplerInfo);
}

vk::Sampler Sampler::Handle()
{
  return *mSampler;
}

struct Texture::Impl
{
  Device& mDevice;
  uint32_t mWidth;
  uint32_t mHeight;
  vk::Format mFormat;
  VkImage mImage;
  VmaAllocation mAllocation;
  VmaAllocationInfo mAllocationInfo;
  vk::UniqueImageView mImageView;

  Impl(Device& device, uint32_t width, uint32_t height, vk::Format format, MemoryUsage memoryUsage)
      : mDevice(device), mWidth(width), mHeight(height), mFormat(format)
  {
    vk::ImageUsageFlags usageFlags =
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    vk::ImageLayout imageLayout = memoryUsage == MemoryUsage::Cpu ? vk::ImageLayout::ePreinitialized
                                                                  : vk::ImageLayout::eUndefined;

    if (memoryUsage != MemoryUsage::Cpu)
    {
      usageFlags |= vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                    vk::ImageUsageFlagBits::eColorAttachment;
    }

    auto imageInfo = vk::ImageCreateInfo()
                         .setImageType(vk::ImageType::e2D)
                         .setExtent({width, height, 1})
                         .setMipLevels(1)
                         .setArrayLayers(1)
                         .setFormat(format)
                         .setTiling(memoryUsage == MemoryUsage::Cpu ? vk::ImageTiling::eLinear
                                                                    : vk::ImageTiling::eOptimal)
                         .setInitialLayout(imageLayout)
                         .setUsage(usageFlags)
                         .setSharingMode(vk::SharingMode::eExclusive)
                         .setSamples(vk::SampleCountFlagBits::e1);

    VkImageCreateInfo vkImageInfo = imageInfo;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = ConvertMemoryUsage(memoryUsage);
    if (vmaCreateImage(device.Allocator(),
                       &vkImageInfo,
                       &allocInfo,
                       &mImage,
                       &mAllocation,
                       &mAllocationInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("Error creating texture");
    }

    if (memoryUsage != MemoryUsage::Cpu)
    {
      auto imageViewInfo = vk::ImageViewCreateInfo()
                               .setImage(mImage)
                               .setFormat(format)
                               .setViewType(vk::ImageViewType::e2D)
                               .setComponents({vk::ComponentSwizzle::eIdentity,
                                               vk::ComponentSwizzle::eIdentity,
                                               vk::ComponentSwizzle::eIdentity,
                                               vk::ComponentSwizzle::eIdentity})
                               .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

      mImageView = device.Handle().createImageViewUnique(imageViewInfo);
    }

    // Transition to eGeneral and clear texture
    // TODO perhaps have initial color or data in the constructor?
    device.Execute([&](CommandEncoder& command) {
      if (memoryUsage != MemoryUsage::Cpu)
      {
        Barrier(command,
                imageLayout,
                vk::AccessFlagBits{},
                vk::ImageLayout::eGeneral,
                vk::AccessFlagBits::eTransferWrite);

        auto clearValue = vk::ClearColorValue().setFloat32({{0.0f, 0.0f, 0.0f, 0.0f}});

        command.Handle().clearColorImage(
            mImage,
            vk::ImageLayout::eGeneral,
            clearValue,
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

        Barrier(command,
                vk::ImageLayout::eGeneral,
                vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eGeneral,
                vk::AccessFlagBits{});
      }
      else
      {
        Barrier(command,
                imageLayout,
                vk::AccessFlagBits{},
                vk::ImageLayout::eGeneral,
                vk::AccessFlagBits{});
      }
    });
  }

  ~Impl()
  {
    if (mImage)
    {
      vmaDestroyImage(mDevice.Allocator(), mImage, mAllocation);
    }
  }

  Impl(Impl&& other)
      : mDevice(other.mDevice)
      , mWidth(other.mWidth)
      , mHeight(other.mHeight)
      , mFormat(other.mFormat)
      , mImage(other.mImage)
      , mAllocation(other.mAllocation)
      , mAllocationInfo(other.mAllocationInfo)
      , mImageView(std::move(other.mImageView))
  {
    other.mImage = vk::Image();
    other.mAllocation = VK_NULL_HANDLE;
  }

  void Clear(CommandEncoder& command, const std::array<int, 4>& colour)
  {
    vk::ClearColorValue colourValue(colour);
    Clear(command, colourValue);
  }

  void Clear(CommandEncoder& command, const std::array<float, 4>& colour)
  {
    vk::ClearColorValue colourValue(colour);
    Clear(command, colourValue);
  }

  void Clear(CommandEncoder& command, vk::ClearColorValue colour)
  {
    // TODO access flags wrong?
    Barrier(command,
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits{},
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits::eTransferWrite);

    command.Handle().clearColorImage(
        mImage,
        vk::ImageLayout::eGeneral,
        colour,
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    Barrier(command,
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits{});
  }

  void CopyFrom(const void* data)
  {
    std::uint64_t bytesPerPixel = GetBytesPerPixel(mFormat);

    // TODO use always mapped functionality of VMA

    VkMemoryPropertyFlags memFlags;
    vmaGetMemoryTypeProperties(mDevice.Allocator(), mAllocationInfo.memoryType, &memFlags);
    if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
      throw std::runtime_error("Not visible image");

    void* pData;
    if (vmaMapMemory(mDevice.Allocator(), mAllocation, &pData) != VK_SUCCESS)
      throw std::runtime_error("Cannot map buffer");

    auto subresource = vk::ImageSubresource()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setMipLevel(0)
                           .setArrayLayer(0);

    auto srcLayout = mDevice.Handle().getImageSubresourceLayout(mImage, subresource);

    const uint8_t* src = (const uint8_t*)data;
    uint8_t* dst = (uint8_t*)pData;

    dst += srcLayout.offset;
    for (uint32_t y = 0; y < mHeight; y++)
    {
      std::memcpy(dst, src, mWidth * bytesPerPixel);

      dst += srcLayout.rowPitch;
      src += mWidth * bytesPerPixel;
    }

    if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
      auto memRange = vk::MappedMemoryRange()
                          .setMemory(mAllocationInfo.deviceMemory)
                          .setOffset(mAllocationInfo.offset)
                          .setSize(mAllocationInfo.size);

      mDevice.Handle().flushMappedMemoryRanges(memRange);
    }

    vmaUnmapMemory(mDevice.Allocator(), mAllocation);
  }

  void CopyTo(void* data)
  {
    std::uint64_t bytesPerPixel = GetBytesPerPixel(mFormat);

    // TODO use always mapped functionality of VMA

    VkMemoryPropertyFlags memFlags;
    vmaGetMemoryTypeProperties(mDevice.Allocator(), mAllocationInfo.memoryType, &memFlags);
    if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
      throw std::runtime_error("Not visible image");

    void* pData;
    if (vmaMapMemory(mDevice.Allocator(), mAllocation, &pData) != VK_SUCCESS)
      throw std::runtime_error("Cannot map buffer");

    if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
      auto memRange = vk::MappedMemoryRange()
                          .setMemory(mAllocationInfo.deviceMemory)
                          .setOffset(mAllocationInfo.offset)
                          .setSize(mAllocationInfo.size);

      mDevice.Handle().invalidateMappedMemoryRanges(memRange);
    }

    auto subresource = vk::ImageSubresource()
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setMipLevel(0)
                           .setArrayLayer(0);

    auto srcLayout = mDevice.Handle().getImageSubresourceLayout(mImage, subresource);

    uint8_t* dst = (uint8_t*)data;
    const uint8_t* src = (const uint8_t*)pData;

    src += srcLayout.offset;
    for (uint32_t y = 0; y < mHeight; y++)
    {
      std::memcpy(dst, src, mWidth * bytesPerPixel);

      src += srcLayout.rowPitch;
      dst += mWidth * bytesPerPixel;
    }

    vmaUnmapMemory(mDevice.Allocator(), mAllocation);
  }

  void CopyFrom(CommandEncoder& command, Texture& srcImage)
  {
    if (!(mWidth == srcImage.GetWidth() && mHeight == srcImage.GetHeight() &&
          mFormat == srcImage.GetFormat()))
    {
      throw std::runtime_error("Invalid source texture to copy");
    }

    srcImage.Barrier(command,
                     vk::ImageLayout::eGeneral,
                     vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead,
                     vk::ImageLayout::eTransferSrcOptimal,
                     vk::AccessFlagBits::eTransferRead);
    Barrier(command,
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits::eTransferWrite);

    auto region = vk::ImageCopy()
                      .setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
                      .setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
                      .setExtent({mWidth, mHeight, 1});

    command.Handle().copyImage(srcImage.Handle(),
                               vk::ImageLayout::eTransferSrcOptimal,
                               mImage,
                               vk::ImageLayout::eTransferDstOptimal,
                               region);

    srcImage.Barrier(command,
                     vk::ImageLayout::eTransferSrcOptimal,
                     vk::AccessFlagBits::eTransferRead,
                     vk::ImageLayout::eGeneral,
                     vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead |
                         vk::AccessFlagBits::eHostRead);

    Barrier(command,
            vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead |
                vk::AccessFlagBits::eHostRead);
  }

  void Barrier(CommandEncoder& command,
               vk::ImageLayout oldLayout,
               vk::AccessFlags srcMask,
               vk::ImageLayout newLayout,
               vk::AccessFlags dstMask)
  {
    TextureBarrier(mImage, command.Handle(), oldLayout, srcMask, newLayout, dstMask);
  }

  vk::ImageView GetView() const { return *mImageView; }

  uint32_t GetWidth() const { return mWidth; }

  uint32_t GetHeight() const { return mHeight; }

  vk::Format GetFormat() const { return mFormat; }

  vk::Image Handle() const { return mImage; }
};

Texture::Texture(Device& device,
                 uint32_t width,
                 uint32_t height,
                 vk::Format format,
                 MemoryUsage memoryUsage)
    : mImpl(std::make_unique<Impl>(device, width, height, format, memoryUsage))
{
}

Texture::Texture(Texture&& other) : mImpl(std::move(other.mImpl)) {}

Texture::~Texture() {}

void Texture::CopyFrom(const void* data)
{
  mImpl->CopyFrom(data);
}

void Texture::CopyTo(void* data)
{
  mImpl->CopyTo(data);
}

void Texture::CopyFrom(CommandEncoder& command, Texture& srcImage)
{
  mImpl->CopyFrom(command, srcImage);
}

void Texture::Barrier(CommandEncoder& command,
                      vk::ImageLayout oldLayout,
                      vk::AccessFlags oldAccess,
                      vk::ImageLayout newLayout,
                      vk::AccessFlags newAccess)
{
  mImpl->Barrier(command, oldLayout, oldAccess, newLayout, newAccess);
}

vk::ImageView Texture::GetView() const
{
  return mImpl->GetView();
}

uint32_t Texture::GetWidth() const
{
  return mImpl->GetWidth();
}

uint32_t Texture::GetHeight() const
{
  return mImpl->GetHeight();
}

vk::Format Texture::GetFormat() const
{
  return mImpl->GetFormat();
}

void Texture::Clear(CommandEncoder& command, const std::array<int, 4>& colour)
{
  mImpl->Clear(command, colour);
}

void Texture::Clear(CommandEncoder& command, const std::array<float, 4>& colour)
{
  mImpl->Clear(command, colour);
}

vk::Image Texture::Handle() const
{
  return mImpl->Handle();
}

}  // namespace Renderer
}  // namespace Vortex2D
