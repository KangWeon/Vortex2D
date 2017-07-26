//
//  RenderTexture.cpp
//  Vortex
//

#include "RenderTexture.h"

namespace Vortex2D { namespace Renderer {

RenderTexture::RenderTexture(const Device& device, uint32_t width, uint32_t height, vk::Format format)
    : RenderTarget(width, height)
    , Texture(device, width, height, format, false)
    , mDevice(device)
{
    // Create render pass
    RenderPass = RenderpassBuilder()
            .Attachement(format)
            .AttachementLoadOp(vk::AttachmentLoadOp::eLoad)
            .AttachementStoreOp(vk::AttachmentStoreOp::eStore)
            // TODO should they both be general?
            .AttachementInitialLayout(vk::ImageLayout::eGeneral)
            .AttachementFinalLayout(vk::ImageLayout::eGeneral)
            .Subpass(vk::PipelineBindPoint::eGraphics)
            .SubpassColorAttachment(vk::ImageLayout::eColorAttachmentOptimal, 0)
            .Dependency(VK_SUBPASS_EXTERNAL, 0)
            .DependencySrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .DependencyDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .DependencySrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
            .DependencyDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .Create(mDevice.Handle());

    // Create framebuffer
    vk::ImageView attachments[] = {*this};

    auto framebufferInfo = vk::FramebufferCreateInfo()
            .setWidth(Width)
            .setHeight(Height)
            .setRenderPass(*RenderPass)
            .setAttachmentCount(1)
            .setPAttachments(attachments)
            .setLayers(1);

    mFramebuffer = device.Handle().createFramebufferUnique(framebufferInfo);

    // Create Command Buffer
    mCmd = mDevice.CreateCommandBuffers(1).at(0);

    // Create fence
    auto fenceInfo = vk::FenceCreateInfo()
            .setFlags(vk::FenceCreateFlagBits::eSignaled);

    mFence = mDevice.Handle().createFenceUnique(fenceInfo);
}

RenderTexture::~RenderTexture()
{
    mDevice.FreeCommandBuffers({mCmd});
}

void RenderTexture::Record(CommandFn commandFn)
{
    mDevice.Handle().waitForFences({*mFence}, true, UINT64_MAX);
    mDevice.Handle().resetFences({*mFence});

    auto bufferBegin = vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    mCmd.begin(bufferBegin);

    auto renderPassBegin = vk::RenderPassBeginInfo()
            .setFramebuffer(*mFramebuffer)
            .setRenderPass(*RenderPass)
            .setRenderArea({{0, 0}, {Width, Height}});

    mCmd.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

    commandFn(mCmd);

    mCmd.endRenderPass();
    mCmd.end();
}

void RenderTexture::Submit()
{
    auto submitInfo = vk::SubmitInfo()
            .setCommandBufferCount(1)
            .setPCommandBuffers(&mCmd);

    mDevice.Queue().submit({submitInfo}, *mFence);
}

}}
