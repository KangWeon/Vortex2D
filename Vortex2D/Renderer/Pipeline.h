//
//  Pipeline.h
//  Vortex2D
//

#ifndef Vortex_Shader_h
#define Vortex_Shader_h

#include <Vortex2D/Renderer/Common.h>
#include <Vortex2D/Renderer/Device.h>
#include <Vortex2D/Renderer/RenderState.h>

#include <vector>
#include <string>

namespace Vortex2D { namespace Renderer {

struct Size
{
    uint32_t x, y;
};

Size GetLocalSize(uint32_t width, uint32_t height);
Size GetWorkSize(uint32_t width, uint32_t height);

class PipelineLayoutBuilder
{
public:
    PipelineLayoutBuilder& DescriptorSetLayout(vk::DescriptorSetLayout layout);
    PipelineLayoutBuilder& PushConstantRange(vk::PushConstantRange range);
    vk::UniquePipelineLayout Create(vk::Device device);

private:
    std::vector<vk::DescriptorSetLayout> mLayouts;
    std::vector<vk::PushConstantRange> mPushConstantRanges;
};

class GraphicsPipeline
{
public:
    // TODO is this format better? or like DescriptorSetLayout?
    class Builder
    {
    public:
        Builder();

        Builder& Shader(vk::ShaderModule shader, vk::ShaderStageFlagBits shaderStage);
        Builder& VertexAttribute(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset);
        Builder& VertexBinding(uint32_t binding, uint32_t stride, vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex);

        Builder& Topology(vk::PrimitiveTopology topology);
        Builder& Layout(vk::PipelineLayout pipelineLayout);
        vk::UniquePipeline Create(vk::Device device, const RenderState& renderState);

    private:
        vk::PipelineMultisampleStateCreateInfo mMultisampleInfo;
        vk::PipelineRasterizationStateCreateInfo mRasterizationInfo;
        vk::PipelineInputAssemblyStateCreateInfo mInputAssembly;
        std::vector<vk::PipelineShaderStageCreateInfo> mShaderStages;
        std::vector<vk::VertexInputBindingDescription> mVertexBindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> mVertexAttributeDescriptions;
        vk::PipelineLayout mPipelineLayout;
        vk::GraphicsPipelineCreateInfo mPipelineInfo;
    };

    GraphicsPipeline();
    GraphicsPipeline(Builder builder);

    void Create(vk::Device device, const RenderState& renderState);
    void Bind(vk::CommandBuffer commandBuffer, const RenderState& renderState);

private:
    Builder mBuilder;
    using PipelineList = std::vector<std::pair<RenderState, vk::UniquePipeline>>;
    PipelineList mPipelines;
};

vk::UniquePipeline MakeComputePipeline(vk::Device device,
                                       vk::ShaderModule shader,
                                       vk::PipelineLayout layout,
                                       vk::SpecializationInfo specializationInfo);

vk::UniquePipeline MakeComputePipeline(vk::Device device,
                                       vk::ShaderModule shader,
                                       vk::PipelineLayout layout,
                                       uint32_t localX = 16,
                                       uint32_t localY = 16);

}}

#endif