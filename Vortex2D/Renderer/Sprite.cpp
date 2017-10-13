//
//  Sprite.cpp
//  Vortex2D
//

#include "Sprite.h"

#include <Vortex2D/Renderer/RenderTarget.h>
#include <Vortex2D/SPIRV/Reflection.h>
#include <Vortex2D/Renderer/CommandBuffer.h>

namespace Vortex2D { namespace Renderer {

AbstractSprite::AbstractSprite(const Device& device, const std::string& fragShaderName, Texture& texture)
    : mDevice(device)
    , mMVPBuffer(device)
    , mVertexBuffer(device, 6, false)
{
    VertexBuffer<Vertex> localBuffer(device, 6, true);
    std::vector<Vertex> vertices = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}},
        {{1.0f, 0.0f}, {texture.GetWidth(), 0.0f}},
        {{0.0f, 1.0f}, {0.0f, texture.GetHeight()}},
        {{1.0f, 0.0f}, {texture.GetWidth(), 0.0f}},
        {{1.0f, 1.0f}, {texture.GetWidth(), texture.GetHeight()}},
        {{0.0f, 1.0f}, {0.0f, texture.GetHeight()}}
    };

    Renderer::CopyFrom(localBuffer, vertices);
    ExecuteCommand(device, [&](vk::CommandBuffer commandBuffer)
    {
        mVertexBuffer.CopyFrom(commandBuffer, localBuffer);
    });

    SPIRV::Reflection reflectionVert(device.GetShaderSPIRV("../Vortex2D/TexturePosition.vert.spv"));
    SPIRV::Reflection reflectionFrag(device.GetShaderSPIRV(fragShaderName));

    static vk::DescriptorSetLayout descriptorLayout = DescriptorSetLayoutBuilder()
            .Binding(reflectionVert.GetDescriptorTypesMap(), reflectionVert.GetShaderStage())
            .Binding(reflectionFrag.GetDescriptorTypesMap(), reflectionFrag.GetShaderStage())
            .Create(device);

    mDescriptorSet = MakeDescriptorSet(device, descriptorLayout);

    // TODO add as parameter
    mSampler = SamplerBuilder().Filter(vk::Filter::eLinear).Create(device.Handle());

    DescriptorSetUpdater(*mDescriptorSet)
            .Bind(reflectionVert.GetDescriptorTypesMap(), {{mMVPBuffer, 0}})
            .Bind(reflectionFrag.GetDescriptorTypesMap(), {{*mSampler, texture, 1}})
            .Update(device.Handle());

    auto pipelineLayoutBuilder = PipelineLayoutBuilder()
            .DescriptorSetLayout(descriptorLayout);

    unsigned pushConstantSize = reflectionFrag.GetPushConstantsSize();
    if (pushConstantSize > 0)
    {
        pipelineLayoutBuilder.PushConstantRange({vk::ShaderStageFlagBits::eFragment, 0, pushConstantSize});
    }

    mPipelineLayout = pipelineLayoutBuilder.Create(device.Handle());

    vk::ShaderModule vertexShader = device.GetShaderModule("../Vortex2D/TexturePosition.vert.spv");
    vk::ShaderModule fragShader = device.GetShaderModule(fragShaderName);

    mPipeline = GraphicsPipeline::Builder()
            .Shader(vertexShader, vk::ShaderStageFlagBits::eVertex)
            .Shader(fragShader, vk::ShaderStageFlagBits::eFragment)
            .VertexAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos))
            .VertexAttribute(1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv))
            .VertexBinding(0, sizeof(Vertex))
            .Layout(*mPipelineLayout);

}

void AbstractSprite::Update(const glm::mat4& projection, const glm::mat4& view)
{
    Renderer::CopyFrom(mMVPBuffer, projection * view * GetTransform());
    ExecuteCommand(mDevice, [&](vk::CommandBuffer commandBuffer)
    {
       mMVPBuffer.Upload(commandBuffer);
    });
}

void AbstractSprite::Initialize(const RenderState& renderState)
{
    mPipeline.Create(mDevice.Handle(), renderState);
}

void AbstractSprite::Draw(vk::CommandBuffer commandBuffer, const RenderState& renderState)
{
    mPipeline.Bind(commandBuffer, renderState);
    commandBuffer.bindVertexBuffers(0, {mVertexBuffer.Handle()}, {0ul});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *mPipelineLayout, 0, {*mDescriptorSet}, {});
    commandBuffer.draw(6, 1, 0, 0);
}

Sprite::Sprite(const Device& device, Texture& texture)
    : AbstractSprite(device, "../Vortex2D/TexturePosition.frag.spv", texture)
{

}

}}
