#include "gfx/gfx.h"
#include "gfx/gfx-constants.h"
#include "gfx/gfx-pipeline.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "gfx-constants-private.h"
#include "render-target-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

template<typename DescriptionType>
void AddDescriptionImplTemplate(std::vector<DescriptionType>& descriptions, DescriptionType description) {
    auto it = std::lower_bound(descriptions.begin(), descriptions.end(), description, 
        [](const DescriptionType& element, const DescriptionType& value) {
            return element.attribute < value.attribute;
        });
    if (it == descriptions.end() || it->attribute != description.attribute) {
        descriptions.emplace(it, description);
    } else {
        *it = description;
    }
}

} // unnamed namespace

namespace wg {

void GfxVertexFactory::AddDescriptionImpl(std::vector<VertexFactoryDescription>& descriptions, VertexFactoryDescription description) {
    AddDescriptionImplTemplate(descriptions, description);
}

void GfxVertexFactory::AddDescriptionImpl(std::vector<VertexBufferDescription>& descriptions, VertexBufferDescription description) {
    AddDescriptionImplTemplate(descriptions, description);
}

GfxVertexFactory::GfxVertexFactory(std::initializer_list<VertexFactoryDescription> initializer_list) {
    for (auto&& description : initializer_list) {
        AddDescriptionImpl(descriptions_, description);
    }
}

void GfxVertexFactory::addDescription(VertexFactoryDescription description) {
    AddDescriptionImpl(descriptions_, description);
}

void GfxVertexFactory::clearDescriptions() {
    descriptions_.clear();
}

void GfxVertexFactory::addVertexBuffer(const std::shared_ptr<VertexBufferBase>& vertex_buffer) {
    vertex_buffers_.emplace_back(vertex_buffer);
}

void GfxVertexFactory::clearVertexBuffers() {
    vertex_buffers_.clear();
}

void GfxVertexFactory::setIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer) {
    index_buffer_ = index_buffer;
}

void GfxVertexFactory::clearIndexBuffer() {
    index_buffer_.reset();
}

bool GfxVertexFactory::valid() const {
    std::vector<VertexBufferDescription> vertex_buffer_descriptions;
    for (auto&& vertex_buffer : vertex_buffers_) {
        for (auto&& description : vertex_buffer->descriptions()) {
            AddDescriptionImpl(vertex_buffer_descriptions, description);
        }
    }
    for (auto&& description : descriptions_) {
        auto it = std::lower_bound(
            vertex_buffer_descriptions.begin(), vertex_buffer_descriptions.end(), description, 
            [](const VertexBufferDescription& element, const VertexFactoryDescription& value) {
                return element.attribute < value.attribute;
            });
        if (it == vertex_buffer_descriptions.end() ||
            it->attribute != description.attribute ||
            it->format != description.format) {
            return false;
        }
    }
    return true;
}

bool GfxVertexFactory::draw_indexed() const {
    return static_cast<bool>(index_buffer_);
}

size_t GfxVertexFactory::vertex_count() const {
    if (vertex_buffers_.empty()) {
        return 0;
    }
    size_t result = std::numeric_limits<size_t>::max();
    for (auto&& vertex_buffer : vertex_buffers_) {
        result = std::min(result, vertex_buffer->vertex_count());
    }
    return result;
}

size_t GfxVertexFactory::index_count() const {
    if (index_buffer_) {
        return index_buffer_->index_count();
    }
    return 0;
}

std::vector<VertexFactoryCombinedDescription> GfxVertexFactory::getCombinedDescriptions() const {
    std::vector<VertexBufferDescription> vertex_buffer_descriptions;
    for (auto&& vertex_buffer : vertex_buffers_) {
        for (auto&& description : vertex_buffer->descriptions()) {
            AddDescriptionImpl(vertex_buffer_descriptions, description);
        }
    }
    std::vector<VertexFactoryCombinedDescription> combined_descriptions;
    for (auto&& description : descriptions_) {
        for (size_t vertex_buffer_index = 0; vertex_buffer_index < vertex_buffers_.size(); ++vertex_buffer_index) {
            const auto& vertex_buffer = vertex_buffers_[vertex_buffer_index];
            for (auto&& vertex_buffer_description : vertex_buffer->descriptions()) {
                if (vertex_buffer_description.attribute == description.attribute &&
                    vertex_buffer_description.format == description.format) {
                            
                    combined_descriptions.emplace_back(VertexFactoryCombinedDescription{
                        .attribute           = description.attribute,
                        .format              = description.format,
                        .location            = description.location,
                        .stride              = vertex_buffer_description.stride,
                        .offset              = vertex_buffer_description.offset,
                        .vertex_buffer_index = vertex_buffer_index,
                    });
                }
            }
        }
    }
    return combined_descriptions;
}

void GfxUniformLayout::addDescription(UniformDescription description) {
    AddDescriptionImplTemplate(descriptions_, description);
}

void GfxUniformLayout::clearDescriptions() {
    descriptions_.clear();
}

void GfxUniformLayout::addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    uniform_buffers_.push_back(uniform_buffer);
}

void GfxUniformLayout::clearUniformBuffers() {
    uniform_buffers_.clear();
}

bool GfxUniformLayout::valid() const {
    std::vector<UniformObjectDescription> uniform_buffer_descriptions;
    for (auto&& uniform_buffer : uniform_buffers_) {
        AddDescriptionImplTemplate(uniform_buffer_descriptions, uniform_buffer->description());
    }
    for (auto&& description : descriptions_) {
        auto it = std::lower_bound(
            uniform_buffer_descriptions.begin(), uniform_buffer_descriptions.end(), description, 
            [](const UniformObjectDescription& element, const UniformDescription& value) {
                return element.attribute < value.attribute;
            });
        if (it == uniform_buffer_descriptions.end() ||
            it->attribute != description.attribute) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<GfxPipeline> GfxPipeline::Create(std::string name) {
    return std::shared_ptr<GfxPipeline>(new GfxPipeline(std::move(name)));
}

GfxPipeline::GfxPipeline(std::string name)
    : name_(std::move(name)) {
}

void Gfx::createPipelineResources(
    const std::shared_ptr<RenderTarget>& render_target,
    const std::shared_ptr<GfxPipeline>& pipeline) {

    logger().info("Creating resources for pipeline \"{}\".", pipeline->name());

    if (!logical_device_) {
        logger().error("Cannot create pipeline resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto* resources = render_target->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot create pipeline because render target resources are not available.");
        return;
    }
    
    // Stages
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    for (auto& shader : pipeline->shaders()) {
        if (auto* shader_resources = shader->impl_->resources.get()) {
            shader_stages.emplace_back(vk::PipelineShaderStageCreateInfo{
                .stage  = GetShaderStageFlag(shader->stage()),
                .module = *shader_resources->shader_module,
                .pName  = shader->entry().c_str()
            });
        }
    }
    
    // Vertex factory
    std::vector<vk::VertexInputBindingDescription> vertex_bindings;
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    std::vector<vk::Buffer> vertex_buffers;
    std::vector<vk::DeviceSize> vertex_buffer_offsets;
    std::map<size_t, uint32_t> vb_index_to_binding;
    for (auto&& description : pipeline->vertex_factory_.getCombinedDescriptions()) {
        uint32_t binding = [&vb_index_to_binding, &description, &vertex_bindings]() {
            auto it = vb_index_to_binding.find(description.vertex_buffer_index);
            if (it == vb_index_to_binding.end()) {
                uint32_t new_binding = static_cast<uint32_t>(vertex_bindings.size());
                vb_index_to_binding[description.vertex_buffer_index] = new_binding;
                vertex_bindings.emplace_back(vk::VertexInputBindingDescription{
                    .binding = new_binding,
                    .stride = description.stride,
                    .inputRate = vk::VertexInputRate::eVertex
                });
                return new_binding;
            } else {
                if (vertex_bindings[it->second].stride != description.stride) {
                    logger().warn("Vertex binding and buffer stride not identical: {} != {}.", 
                        vertex_bindings[it->second].stride, description.stride);
                }
                return it->second;
            }
        }();

        vertex_attributes.emplace_back(vk::VertexInputAttributeDescription{
            .location = description.location,
            .binding = binding,
            .format = gfx_formats::ToVkFormat(description.format),
            .offset = description.offset
        });

        auto&& vertex_buffer = pipeline->vertex_factory_.vertex_buffers_[description.vertex_buffer_index];
        if (auto vertex_buffer_resources = vertex_buffer->impl_->resources.get()) {
            vertex_buffers.emplace_back(*vertex_buffer_resources->buffer);
            vertex_buffer_offsets.emplace_back(0);
        }
    }

    
    vk::Buffer index_buffer = nullptr;
    vk::DeviceSize index_buffer_offset = 0;
    vk::IndexType index_type = {};
    const bool draw_indexed = pipeline->vertex_factory_.draw_indexed();
    if (draw_indexed) {
        if (auto index_buffer_resources = pipeline->vertex_factory_.index_buffer_->impl_->resources.get()) {
            index_buffer = *index_buffer_resources->buffer;
        }
        index_type = index_types::ToVkIndexType(pipeline->vertex_factory_.index_buffer_->index_type());
    }

    auto vertex_input_create_info = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(vertex_bindings)
        .setVertexAttributeDescriptions(vertex_attributes);

    auto input_assembly_create_info = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false
    };

    // Pipeline state
    auto [width, height] = render_target->extent();
    auto viewport = vk::Viewport{
        .x        = 0.f,
        .y        = 0.f,
        .width    = static_cast<float>(width),
        .height   = static_cast<float>(height),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };
    auto viewports = std::array{ viewport };

    auto scissor = vk::Rect2D{
        .offset = { 0, 0 },
        .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
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
    
    std::vector<vk::DescriptorSetLayout> set_layouts;
    
    // Uniform layout
    if (!pipeline->uniform_layout_.descriptions_.empty()) {
        std::vector<vk::DescriptorSetLayoutBinding> uniform_bindings;
        uniform_bindings.reserve(pipeline->uniform_layout_.descriptions_.size());
        for (auto&& description : pipeline->uniform_layout_.descriptions_) {
            uniform_bindings.emplace_back(vk::DescriptorSetLayoutBinding{
                .binding         = description.binding,
                .descriptorType  = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags      = GetShaderStageFlags(description.stages)
            });
        }
        auto uniform_layout_create_info = vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(uniform_bindings);
        resources->uniform_layout = logical_device_->impl_->vk_device.createDescriptorSetLayout(
            uniform_layout_create_info);
        set_layouts.push_back(*resources->uniform_layout);
    }

    auto push_constant_ranges = std::array<vk::PushConstantRange, 0>{};
    auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayouts(set_layouts)
        .setPushConstantRanges(push_constant_ranges);
    resources->pipeline_layout.emplace_back( 
        logical_device_->impl_->vk_device.createPipelineLayout(pipeline_layout_create_info));

    // Pipeline
    auto pipeline_create_info = vk::GraphicsPipelineCreateInfo{
        .pVertexInputState   = &vertex_input_create_info,
        .pInputAssemblyState = &input_assembly_create_info,
        .pViewportState      = &viewport_create_info,
        .pRasterizationState = &rasterization_create_info,
        .pMultisampleState   = &multisample_create_info,
        .pDepthStencilState  = &depth_stencil_create_info,
        .pColorBlendState    = &color_blend_create_info,
        .pDynamicState       = &dynamic_state_create_info,
        .layout              = *resources->pipeline_layout.back(),
        .renderPass          = *resources->render_pass,
        .subpass             = 0,
    }   .setStages(shader_stages);

    resources->pipeline.emplace_back(
        logical_device_->impl_->vk_device.createGraphicsPipeline({ nullptr }, pipeline_create_info));

    resources->draw_command_resources.emplace_back(DrawCommandResources{
        .pipeline = *resources->pipeline.back(),
        .vertex_buffers = vertex_buffers,
        .vertex_buffer_offsets = vertex_buffer_offsets,
        .draw_indexed = draw_indexed,
        .index_buffer = index_buffer,
        .index_buffer_offset = index_buffer_offset,
        .index_type = index_type
    });
}

} // namespace wg