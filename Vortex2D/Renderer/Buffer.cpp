//
//  Buffer.cpp
//  Vortex2D
//

#include "Buffer.h"

#include <Vortex2D/Renderer/CommandBuffer.h>
#include <Vortex2D/Renderer/Device.h>
#include <Vortex2D/Renderer/Texture.h>

namespace Vortex2D
{
namespace Renderer
{
void BufferBarrier(vk::Buffer buffer,
                   vk::CommandBuffer commandBuffer,
                   vk::AccessFlags oldAccess,
                   vk::AccessFlags newAccess)
{
  auto bufferMemoryBarriers = vk::BufferMemoryBarrier()
                                  .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                  .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                  .setBuffer(buffer)
                                  .setSize(VK_WHOLE_SIZE)
                                  .setSrcAccessMask(oldAccess)
                                  .setDstAccessMask(newAccess);

  commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eAllCommands,
                                {},
                                nullptr,
                                bufferMemoryBarriers,
                                nullptr);
}

GenericBuffer::GenericBuffer(const Device& device,
                             vk::BufferUsageFlags usageFlags,
                             VmaMemoryUsage memoryUsage,
                             vk::DeviceSize deviceSize)
    : mDevice(device)
    , mSize(deviceSize)
    , mUsageFlags(usageFlags | vk::BufferUsageFlagBits::eTransferDst |
                  vk::BufferUsageFlagBits::eTransferSrc)
    , mMemoryUsage(memoryUsage)
{
  Create();
}

GenericBuffer::~GenericBuffer()
{
  if (mBuffer != VK_NULL_HANDLE)
  {
    vmaDestroyBuffer(mDevice.Allocator(), mBuffer, mAllocation);
  }
}

GenericBuffer::GenericBuffer(GenericBuffer&& other)
    : mDevice(other.mDevice)
    , mSize(other.mSize)
    , mBuffer(other.mBuffer)
    , mAllocation(other.mAllocation)
    , mAllocationInfo(other.mAllocationInfo)
{
  other.mBuffer = VK_NULL_HANDLE;
  other.mAllocation = VK_NULL_HANDLE;
  other.mSize = 0;
}

void GenericBuffer::Create()
{
  auto bufferInfo = vk::BufferCreateInfo()
                        .setSize(mSize)
                        .setUsage(mUsageFlags)
                        .setSharingMode(vk::SharingMode::eExclusive);

  VkBufferCreateInfo vkBufferInfo = bufferInfo;
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = mMemoryUsage;
  if (vmaCreateBuffer(mDevice.Allocator(),
                      &vkBufferInfo,
                      &allocInfo,
                      &mBuffer,
                      &mAllocation,
                      &mAllocationInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("Error creating buffer");
  }

  // TODO we shouldn't have to clear always in the constructor
  mDevice.Execute([&](vk::CommandBuffer commandBuffer) { Clear(commandBuffer); });
}

vk::Buffer GenericBuffer::Handle() const
{
  return mBuffer;
}

vk::DeviceSize GenericBuffer::Size() const
{
  return mSize;
}

void GenericBuffer::Resize(vk::DeviceSize size)
{
  if (mBuffer != VK_NULL_HANDLE)
  {
    vmaDestroyBuffer(mDevice.Allocator(), mBuffer, mAllocation);
  }

  mSize = size;
  Create();
}

void GenericBuffer::CopyFrom(vk::CommandBuffer commandBuffer, GenericBuffer& srcBuffer)
{
  if (mSize != srcBuffer.mSize)
  {
    throw std::runtime_error("Cannot copy buffers of different sizes");
  }

  // TODO improve barriers
  srcBuffer.Barrier(
      commandBuffer, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead);
  Barrier(commandBuffer, vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite);

  auto region = vk::BufferCopy().setSize(mSize);

  commandBuffer.copyBuffer(srcBuffer.Handle(), mBuffer, region);

  Barrier(commandBuffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
  srcBuffer.Barrier(
      commandBuffer, vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead);
}

void GenericBuffer::CopyFrom(vk::CommandBuffer commandBuffer, Texture& srcTexture)
{
  // TODO check if it can be copied

  srcTexture.Barrier(commandBuffer,
                     vk::ImageLayout::eGeneral,
                     vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite,
                     vk::ImageLayout::eTransferSrcOptimal,
                     vk::AccessFlagBits::eTransferRead);

  auto info = vk::BufferImageCopy()
                  .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
                  .setImageExtent({srcTexture.GetWidth(), srcTexture.GetHeight(), 1});

  commandBuffer.copyImageToBuffer(
      srcTexture.mImage, vk::ImageLayout::eTransferSrcOptimal, mBuffer, info);

  srcTexture.Barrier(commandBuffer,
                     vk::ImageLayout::eTransferSrcOptimal,
                     vk::AccessFlagBits::eTransferRead,
                     vk::ImageLayout::eGeneral,
                     vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead);

  Barrier(commandBuffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
}

void GenericBuffer::Barrier(vk::CommandBuffer commandBuffer,
                            vk::AccessFlags oldAccess,
                            vk::AccessFlags newAccess)
{
  BufferBarrier(mBuffer, commandBuffer, oldAccess, newAccess);
}

void GenericBuffer::Clear(vk::CommandBuffer commandBuffer)
{
  Barrier(commandBuffer, vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite);
  commandBuffer.fillBuffer(mBuffer, 0, VK_WHOLE_SIZE, 0);
  Barrier(commandBuffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
}

void GenericBuffer::CopyFrom(uint32_t offset, const void* data, uint32_t size)
{
  // TODO use always mapped functionality of VMA

  VkMemoryPropertyFlags memFlags;
  vmaGetMemoryTypeProperties(mDevice.Allocator(), mAllocationInfo.memoryType, &memFlags);
  if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
    throw std::runtime_error("Not visible buffer");

  void* pData;
  if (vmaMapMemory(mDevice.Allocator(), mAllocation, &pData) != VK_SUCCESS)
    throw std::runtime_error("Cannot map buffer");

  std::memcpy((uint8_t*)pData + offset, data, size);

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

void GenericBuffer::CopyTo(uint32_t offset, void* data, uint32_t size)
{
  // TODO use always mapped functionality of VMA

  VkMemoryPropertyFlags memFlags;
  vmaGetMemoryTypeProperties(mDevice.Allocator(), mAllocationInfo.memoryType, &memFlags);
  if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
    throw std::runtime_error("Not visible buffer");

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

  std::memcpy(data, (uint8_t*)pData + offset, size);

  vmaUnmapMemory(mDevice.Allocator(), mAllocation);
}

}  // namespace Renderer
}  // namespace Vortex2D
