#include "gfx/gfx.h"
#include "gfx/gfx-constants.h"
#include "gfx/gfx-pipeline.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "gfx-constants-private.h"
#include "gfx-pipeline-private.h"
#include "render-target-private.h"

#include <iterator>

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

GfxUniformLayout& GfxUniformLayout::addDescription(UniformDescription description) {
    AddDescriptionImplTemplate(descriptions_, description);
    return *this;
}

void GfxUniformLayout::clearDescriptions() {
    descriptions_.clear();
}

GfxUniformLayout& GfxUniformLayout::addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    uniform_buffers_.push_back(uniform_buffer);
    return *this;
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
    : name_(std::move(name)), impl_(std::make_unique<Impl>()) {
}

void Gfx::createPipelineResources(
    const std::shared_ptr<GfxPipeline>& pipeline) {
    
    logger().info("Creating resources for pipeline \"{}\".", pipeline->name());

    if (!logical_device_) {
        logger().error("Cannot create pipeline resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto resources = std::make_unique<GfxPipelineResources>();

    // Stages
    for (auto& shader : pipeline->shaders()) {
        if (auto* shader_resources = shader->impl_->resources.get()) {
            resources->shader_stages.emplace_back(vk::PipelineShaderStageCreateInfo{
                .stage  = GetShaderStageFlag(shader->stage()),
                .module = *shader_resources->shader_module,
                .pName  = shader->entry().c_str()
            });
        }
    }
    
    // Vertex factory
    resources->vertex_count = static_cast<uint32_t>(pipeline->vertex_factory_.vertex_count());
    std::map<size_t, uint32_t> vb_index_to_binding;
    for (auto&& description : pipeline->vertex_factory_.getCombinedDescriptions()) {
        uint32_t binding = [&vb_index_to_binding, &description, &resources]() {
            auto it = vb_index_to_binding.find(description.vertex_buffer_index);
            if (it == vb_index_to_binding.end()) {
                uint32_t new_binding = static_cast<uint32_t>(resources->vertex_bindings.size());
                vb_index_to_binding[description.vertex_buffer_index] = new_binding;
                resources->vertex_bindings.emplace_back(vk::VertexInputBindingDescription{
                    .binding = new_binding,
                    .stride = description.stride,
                    .inputRate = vk::VertexInputRate::eVertex
                });
                return new_binding;
            } else {
                if (resources->vertex_bindings[it->second].stride != description.stride) {
                    logger().warn("Vertex binding and buffer stride not identical: {} != {}.", 
                        resources->vertex_bindings[it->second].stride, description.stride);
                }
                return it->second;
            }
        }();

        resources->vertex_attributes.emplace_back(vk::VertexInputAttributeDescription{
            .location = description.location,
            .binding = binding,
            .format = gfx_formats::ToVkFormat(description.format),
            .offset = description.offset
        });

        auto&& vertex_buffer = pipeline->vertex_factory_.vertex_buffers_[description.vertex_buffer_index];
        if (auto vertex_buffer_resources = vertex_buffer->impl_->resources.get()) {
            resources->vertex_buffers.emplace_back(*vertex_buffer_resources->buffer);
            resources->vertex_buffer_offsets.emplace_back(0);
        }
    }

    resources->draw_indexed = pipeline->vertex_factory_.draw_indexed();
    if (resources->draw_indexed) {
        if (auto index_buffer_resources = pipeline->vertex_factory_.index_buffer_->impl_->resources.get()) {
            resources->index_buffer = *index_buffer_resources->buffer;
        }
        resources->index_buffer_offset = 0;
        resources->index_type = index_types::ToVkIndexType(pipeline->vertex_factory_.index_buffer_->index_type());
        resources->index_count = static_cast<uint32_t>(pipeline->vertex_factory_.index_count());
    }

    resources->vertex_input_create_info = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(resources->vertex_bindings)
        .setVertexAttributeDescriptions(resources->vertex_attributes);

    resources->input_assembly_create_info = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false
    };

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
        auto set_layout_create_info = vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(uniform_bindings);
        resources->uniform_layout = 
            logical_device_->impl_->vk_device.createDescriptorSetLayout(set_layout_create_info);
        set_layouts.push_back(*resources->uniform_layout);
    }

    auto push_constant_ranges = std::array<vk::PushConstantRange, 0>{};
    auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayouts(set_layouts)
        .setPushConstantRanges(push_constant_ranges);
    resources->pipeline_layout = 
        logical_device_->impl_->vk_device.createPipelineLayout(pipeline_layout_create_info);

    auto viewport = vk::Viewport{
        .x        = 0.f, // will multiply by actual width
        .y        = 0.f, // will multiply by actual height
        .width    = 1.f, // will multiply by actual width
        .height   = 1.f, // will multiply by actual height
        .minDepth = 0.f,
        .maxDepth = 1.f
    };
    resources->viewports = { viewport };

    auto scissor = vk::Viewport{
        .x        = 0.f, // will multiply by actual width
        .y        = 0.f, // will multiply by actual height
        .width    = 1.f, // will multiply by actual width
        .height   = 1.f, // will multiply by actual height
    };
    resources->scissors = { scissor };

    resources->rasterization_create_info = vk::PipelineRasterizationStateCreateInfo{
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

    resources->multisample_create_info = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples  = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable   = false,
        .minSampleShading      = 1.f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable      = false
    };

    resources->depth_stencil_create_info = vk::PipelineDepthStencilStateCreateInfo{
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
    resources->color_blend_attachments = { color_blend_attachment };

    resources->color_blend_create_info = vk::PipelineColorBlendStateCreateInfo{
        .logicOpEnable  = false,
        .logicOp        = vk::LogicOp::eCopy,
        .blendConstants = std::array{ 0.f, 0.f, 0.f, 0.f }
    }   .setAttachments(resources->color_blend_attachments);

    resources->dynamic_states = {};
    resources->dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{}
        .setDynamicStates(resources->dynamic_states);

    pipeline->impl_->resources = 
        logical_device_->impl_->gfx_pipeline_resources.store(std::move(resources));
}

void Gfx::createPipelineResourcesForRenderTarget(
    const std::shared_ptr<RenderTarget>& render_target,
    const std::shared_ptr<GfxPipeline>& pipeline) {

    logger().info("Creating pipeline resources \"{}\" for render target.", pipeline->name());

    if (!logical_device_) {
        logger().error("Cannot create pipeline resources for render target because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto* resources = render_target->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot create pipeline resources for render target because render target resources are not available.");
        return;
    }

    auto* pipeline_resources = pipeline->impl_->resources.get();
    if (!pipeline_resources) {
        logger().error("Cannot create pipeline resources for render target because pipeline resources are not available.");
        return;
    }
    
    // Pipeline state
    auto [width, height] = render_target->extent();
    std::vector<vk::Viewport> viewports;
    viewports.reserve(pipeline_resources->viewports.size());
    for (auto&& viewport : pipeline_resources->viewports) {
        viewports.emplace_back(viewport);
        viewports.back().x *= static_cast<float>(width);
        viewports.back().y *= static_cast<float>(height);
        viewports.back().width *= static_cast<float>(width);
        viewports.back().height *= static_cast<float>(height);
    }
    
    std::vector<vk::Rect2D> scissors;
    scissors.reserve(pipeline_resources->scissors.size());
    for (auto&& scissor : pipeline_resources->scissors) {
        scissors.emplace_back(vk::Rect2D{
            .offset = { 
                static_cast<int32_t>(scissor.x * width), 
                static_cast<int32_t>(scissor.y * height)
            },
            .extent = {
                static_cast<uint32_t>(scissor.width * width), 
                static_cast<uint32_t>(scissor.height * height)
            }
        });
    }

    auto viewport_create_info = vk::PipelineViewportStateCreateInfo{}
        .setViewports(viewports)
        .setScissors(scissors);

    auto& render_target_pipeline_resources = resources->pipeline_resources.emplace_back();

    // Pipeline
    auto pipeline_create_info = vk::GraphicsPipelineCreateInfo{
        .pVertexInputState   = &pipeline_resources->vertex_input_create_info,
        .pInputAssemblyState = &pipeline_resources->input_assembly_create_info,
        .pViewportState      = &viewport_create_info,
        .pRasterizationState = &pipeline_resources->rasterization_create_info,
        .pMultisampleState   = &pipeline_resources->multisample_create_info,
        .pDepthStencilState  = &pipeline_resources->depth_stencil_create_info,
        .pColorBlendState    = &pipeline_resources->color_blend_create_info,
        .pDynamicState       = &pipeline_resources->dynamic_state_create_info,
        .layout              = *pipeline_resources->pipeline_layout,
        .renderPass          = *resources->render_pass,
        .subpass             = 0,
    }   .setStages(pipeline_resources->shader_stages);
    
    render_target_pipeline_resources.pipeline =
        logical_device_->impl_->vk_device.createGraphicsPipeline({ nullptr }, pipeline_create_info);

    // Descriptor pool
    size_t image_count = resources->command_buffers.size();
    uint32_t uniform_descriptors_count = static_cast<uint32_t>(image_count);
    auto descriptor_pool_sizes = std::array{ vk::DescriptorPoolSize{
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = uniform_descriptors_count
    }};
    auto descriptor_pool_create_info = vk::DescriptorPoolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = uniform_descriptors_count
    }   .setPoolSizes(descriptor_pool_sizes);
    render_target_pipeline_resources.descriptor_pool = 
        logical_device_->impl_->vk_device.createDescriptorPool(descriptor_pool_create_info);

    // Descriptor
    resources->draw_command_resources.emplace_back();

    std::vector<vk::DescriptorSetLayout> set_layouts(
        image_count, *pipeline_resources->uniform_layout);
    
    auto descriptor_pool_alloc_info = vk::DescriptorSetAllocateInfo{
        .descriptorPool = *render_target_pipeline_resources.descriptor_pool
    }   .setSetLayouts(set_layouts);
    render_target_pipeline_resources.descriptor_sets = 
        logical_device_->impl_->vk_device.allocateDescriptorSets(descriptor_pool_alloc_info);
        
    for (size_t i = 0; i < image_count; ++i) {
        resources->draw_command_resources.back().emplace_back(RenderTargetDrawCommandResources{
            .pipeline        = *render_target_pipeline_resources.pipeline,
            .pipeline_layout = *pipeline_resources->pipeline_layout,
            .descriptor_set  = *render_target_pipeline_resources.descriptor_sets[i]
        });
    }
    
    for (size_t i = 0; i < image_count; ++i) {
        for (size_t j = 0; j < pipeline->uniform_layout().descriptions_.size(); ++j) {
            auto& description = pipeline->uniform_layout().descriptions_[j];
            auto& buffer = pipeline->uniform_layout().uniform_buffers_[j];
            std::vector<vk::DescriptorBufferInfo> buffer_info;
            if (auto* buffer_resources = buffer->impl_->resources.get()) {
                buffer_info.emplace_back(vk::DescriptorBufferInfo{
                    .buffer = *buffer_resources->buffer,
                    .offset = 0,
                    .range  = buffer->data_size()
                });
            }
            auto write_description_set = vk::WriteDescriptorSet{
                .dstSet          = *render_target_pipeline_resources.descriptor_sets[i],
                .dstBinding      = description.binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer
            }   .setBufferInfo(buffer_info);
            logical_device_->impl_->vk_device.updateDescriptorSets({ write_description_set }, {});
        }
    }
}

} // namespace wg