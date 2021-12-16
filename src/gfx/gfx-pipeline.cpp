#include "gfx/gfx.h"
#include "gfx/gfx-pipeline.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "gfx-pipeline-private.h"

namespace {
    
auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::shared_ptr<GfxPipeline> GfxPipeline::Create(std::string name) {
    return std::shared_ptr<GfxPipeline>(new GfxPipeline(std::move(name)));
}

GfxPipeline::GfxPipeline(std::string name)
    : name_(std::move(name)), impl_(std::make_unique<GfxPipeline::Impl>()) {
}

void Gfx::createPipelineResources(const std::shared_ptr<GfxPipeline>& pipeline) {

    logger().info("Creating resources for pipeline \"{}\".", pipeline->name());
    
    auto resources = std::make_unique<GfxPipelineResources>();
    
    // Vertex factory
    std::vector<vk::VertexInputBindingDescription> vertex_bindings;
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    auto vertex_input_create_info = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(vertex_bindings)
        .setVertexAttributeDescriptions(vertex_attributes);

    auto input_assembly_create_info = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false
    };

    // Pipeline state
    auto viewport = vk::Viewport{
        .x        = 0.f,
        .y        = 0.f,
        .width    = 800.f,
        .height   = 600.f,
        .minDepth = 0.f,
        .maxDepth = 1.f
    };
    auto viewports = std::array{ viewport };

    auto scissor = vk::Rect2D{
        .offset = {0, 0},
        .extent = {800u, 600u}
    };
    auto scissors = std::array{ scissor };

    auto viewport_create_info = vk::PipelineViewportStateCreateInfo{}
        .setViewports(viewports)
        .setScissors(scissors);

    auto rasterization_create_info = vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable        = false,
        .rasterizerDiscardEnable = false,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = false,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp          = 0.f,
        .depthBiasSlopeFactor    = 0.f,
        .lineWidth               = 1.f,
    };

    auto multisample_create_info = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples  = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable   = false,
        .minSampleShading      = 1.f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable      = false
    };

    auto depth_stencil_create_info = vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable       = false,
        .depthWriteEnable      = false,
        .depthCompareOp        = {},
        .depthBoundsTestEnable = false,
        .stencilTestEnable     = false,
        .front                 = {},
        .back                  = {},
        .minDepthBounds        = 0.f,
        .maxDepthBounds        = 0.f
    };

    auto color_blend_attachment = vk::PipelineColorBlendAttachmentState{
        .blendEnable         = false,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR |
                               vk::ColorComponentFlagBits::eG |
                               vk::ColorComponentFlagBits::eB |
                               vk::ColorComponentFlagBits::eA,
    };
    auto color_blend_attachments = std::array{ color_blend_attachment };

    auto color_blend_create_info = vk::PipelineColorBlendStateCreateInfo{
        .logicOpEnable  = false,
        .logicOp        = vk::LogicOp::eCopy,
        .blendConstants = std::array{ 0.f, 0.f, 0.f, 0.f }
    }   .setAttachments(color_blend_attachments);

    auto dynamic_states = std::array<vk::DynamicState, 0>{};
    auto dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{}
        .setDynamicStates(dynamic_states);
    
    // Uniform layout
    auto set_layouts = std::array<vk::DescriptorSetLayout, 0>{};
    auto push_constant_ranges = std::array<vk::PushConstantRange, 0>{};
    auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayouts(set_layouts)
        .setPushConstantRanges(push_constant_ranges);
    resources->pipeline_layout = 
        logical_device_->impl_->vk_device.createPipelineLayout(pipeline_layout_create_info);

    pipeline->impl_->resource_handle = 
        logical_device_->impl_->pipeline_resources.store(std::move(resources));
    pipeline->valid_ = true;
}

} // namespace wg