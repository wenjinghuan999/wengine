#include "gfx/gfx-pipeline.h"

#include "common/config.h"
#include "common/logger.h"
#include "gfx/gfx.h"
#include "gfx/gfx-buffer.h"
#include "gfx-private.h"
#include "draw-command-private.h"
#include "gfx-constants-private.h"
#include "gfx-pipeline-private.h"
#include "render-target-private.h"

#include <iterator>

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

GfxVertexFactory::GfxVertexFactory(std::initializer_list<VertexFactoryDescription> initializer_list) {
    for (auto&& description : initializer_list) {
        AddDescriptionByAttributeImplTemplate(descriptions_, description);
    }
}

void GfxVertexFactory::addDescription(VertexFactoryDescription description) {
    AddDescriptionByAttributeImplTemplate(descriptions_, description);
}

void GfxVertexFactory::clearDescriptions() {
    descriptions_.clear();
}

GfxUniformLayout& GfxUniformLayout::addDescription(UniformDescription description) {
    AddDescriptionByAttributeImplTemplate(descriptions_, description);
    return *this;
}

void GfxUniformLayout::clearDescriptions() {
    descriptions_.clear();
}

GfxSamplerLayout& GfxSamplerLayout::addDescription(SamplerDescription description) {
    AddDescriptionByBindingImplTemplate(descriptions_, description);
    return *this;
}

void GfxSamplerLayout::clearDescriptions() {
    descriptions_.clear();
}

std::shared_ptr<GfxPipeline> GfxPipeline::Create() {
    return std::shared_ptr<GfxPipeline>(new GfxPipeline());
}

GfxPipeline::GfxPipeline()
    : impl_(std::make_unique<Impl>()) {
}

void Gfx::createPipelineResources(
    const std::shared_ptr<GfxPipeline>& pipeline
) {

    if (!logical_device_) {
        logger().error("Cannot create pipeline resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto resources = std::make_unique<GfxPipelineResources>();

    // Stages
    for (auto& shader : pipeline->shaders()) {
        resources->shader_stages.emplace_back(shader->impl_->shader_stage_create_info);
    }

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;

    // Uniform layout
    if (!pipeline->uniform_layout_.descriptions_.empty()) {
        for (auto&& description : pipeline->uniform_layout_.descriptions_) {
            layout_bindings.emplace_back(
                vk::DescriptorSetLayoutBinding{
                    .binding            = description.binding,
                    .descriptorType     = vk::DescriptorType::eUniformBuffer,
                    .descriptorCount    = 1,
                    .stageFlags         = GetShaderStageFlags(description.stages),
                    .pImmutableSamplers = nullptr
                }
            );
        }
    }

    // Sampler layout
    if (!pipeline->sampler_layout_.descriptions_.empty()) {
        for (auto&& description : pipeline->sampler_layout_.descriptions_) {
            layout_bindings.emplace_back(
                vk::DescriptorSetLayoutBinding{
                    .binding            = description.binding,
                    .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
                    .descriptorCount    = 1,
                    .stageFlags         = GetShaderStageFlags(description.stages),
                    .pImmutableSamplers = nullptr
                }
            );
        }
    }

    std::vector<vk::DescriptorSetLayout> set_layouts;
    if (!layout_bindings.empty()) {
        auto set_layout_create_info = vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(layout_bindings);
        resources->set_layout =
            logical_device_->impl_->vk_device.createDescriptorSetLayout(set_layout_create_info);
        set_layouts.push_back(*resources->set_layout);
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
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = false,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp          = 0.f,
        .depthBiasSlopeFactor    = 0.f,
        .lineWidth               = 1.f,
    };

    float min_sample_shading = 0.f;
    if (features_manager_.feature_enabled(gfx_features::sample_shading)) {
        min_sample_shading = pipeline->pipeline_state_.min_sample_shading;
    }

    resources->multisample_create_info = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples  = static_cast<vk::SampleCountFlagBits>(setup_.msaa_samples),
        .sampleShadingEnable   = min_sample_shading > 0.f,
        .minSampleShading      = min_sample_shading,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable      = false
    };

    resources->depth_stencil_create_info = vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable       = true,
        .depthWriteEnable      = true,
        .depthCompareOp        = vk::CompareOp::eLess,
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
    }
        .setAttachments(resources->color_blend_attachments);

    resources->dynamic_states = {};
    resources->dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{}
        .setDynamicStates(resources->dynamic_states);

    pipeline->impl_->resources =
        logical_device_->impl_->gfx_pipeline_resources.store(std::move(resources));
}

void Gfx::createDrawCommandResourcesForRenderTarget(
    const std::shared_ptr<RenderTarget>& render_target,
    const std::shared_ptr<DrawCommand>& draw_command
) {
    logger().info(
        "Creating resources of draw command \"{}\" for render target \"{}\".",
        draw_command->name(), render_target->name()
    );
    auto& pipeline = draw_command->pipeline_;

    if (!pipeline) {
        logger().error("Cannot create draw command resources because pipeline is not available.");
        return;
    }

    if (!logical_device_) {
        logger().error("Cannot create draw command resources for render target because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto* resources = render_target->impl_->resources.data();
    if (!resources) {
        logger().error("Cannot create draw command resources for render target because render target resources are not available.");
        return;
    }

    auto* pipeline_resources = pipeline->impl_->resources.data();
    if (!pipeline_resources) {
        logger().error("Cannot create draw command resources for render target because pipeline resources are not available.");
        return;
    }

    auto* impl = draw_command->getImpl();
    if (!impl) {
        logger().error("Cannot create draw command resources for render target because draw command impl are not available.");
        return;
    }

    // Pipeline state
    auto[width, height] = render_target->extent();
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
        scissors.emplace_back(
            vk::Rect2D{
                .offset = {
                    static_cast<int32_t>(scissor.x * width),
                    static_cast<int32_t>(scissor.y * height)
                },
                .extent = {
                    static_cast<uint32_t>(scissor.width * width),
                    static_cast<uint32_t>(scissor.height * height)
                }
            }
        );
    }

    auto viewport_create_info = vk::PipelineViewportStateCreateInfo{}
        .setViewports(viewports)
        .setScissors(scissors);

    auto& render_target_pipeline_resources = resources->pipeline_resources.emplace_back();

    // Pipeline
    auto pipeline_create_info = vk::GraphicsPipelineCreateInfo{
        .pVertexInputState   = &impl->vertex_input_create_info,
        .pInputAssemblyState = &impl->input_assembly_create_info,
        .pViewportState      = &viewport_create_info,
        .pRasterizationState = &pipeline_resources->rasterization_create_info,
        .pMultisampleState   = &pipeline_resources->multisample_create_info,
        .pDepthStencilState  = &pipeline_resources->depth_stencil_create_info,
        .pColorBlendState    = &pipeline_resources->color_blend_create_info,
        .pDynamicState       = &pipeline_resources->dynamic_state_create_info,
        .layout              = *pipeline_resources->pipeline_layout,
        .renderPass          = *resources->render_pass,
        .subpass             = 0,
    }
        .setStages(pipeline_resources->shader_stages);

    render_target_pipeline_resources.pipeline =
        logical_device_->impl_->vk_device.createGraphicsPipeline({ nullptr }, pipeline_create_info);

    // Descriptor pool
    size_t image_count = resources->framebuffer_resources.size();
    std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
    if (*pipeline_resources->set_layout) {
        uint32_t uniform_descriptors_count =
            static_cast<uint32_t>(pipeline->uniform_layout().descriptions().size() * image_count);
        if (uniform_descriptors_count > 0) {
            descriptor_pool_sizes.emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eUniformBuffer,
                    .descriptorCount = uniform_descriptors_count
                }
            );
        }
        uint32_t sampler_descriptors_count =
            static_cast<uint32_t>(pipeline->sampler_layout().descriptions().size() * image_count);
        if (sampler_descriptors_count > 0) {
            descriptor_pool_sizes.emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eCombinedImageSampler,
                    .descriptorCount = sampler_descriptors_count
                }
            );
        }
    }

    auto descriptor_pool_create_info = vk::DescriptorPoolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<uint32_t>(image_count)
    }
        .setPoolSizes(descriptor_pool_sizes);

    render_target_pipeline_resources.descriptor_pool =
        logical_device_->impl_->vk_device.createDescriptorPool(descriptor_pool_create_info);

    // Descriptor
    resources->draw_command_resources.emplace_back();

    std::vector<vk::DescriptorSetLayout> set_layouts;
    for (int i = 0; i < image_count; ++i) {
        if (*pipeline_resources->set_layout) {
            set_layouts.push_back(*pipeline_resources->set_layout);
        }
    }

    if (!set_layouts.empty()) {
        auto descriptor_pool_alloc_info = vk::DescriptorSetAllocateInfo{
            .descriptorPool = *render_target_pipeline_resources.descriptor_pool
        }
            .setSetLayouts(set_layouts);

        render_target_pipeline_resources.descriptor_sets =
            logical_device_->impl_->vk_device.allocateDescriptorSets(descriptor_pool_alloc_info);
    }

    // Create draw command resources
    for (size_t i = 0; i < image_count; ++i) {
        std::vector<std::shared_ptr<UniformBufferBase>> gpu_uniforms;
        for (auto&&[attribute, cpu_uniform] : draw_command->uniform_buffers_) {
            auto& gpu_uniform = gpu_uniforms.emplace_back(UniformBufferBase::Create(attribute));
            createUniformBufferResources(gpu_uniform);
            if (cpu_uniform->has_cpu_data()) {
                commitReferenceBuffer(cpu_uniform, gpu_uniform);
            }
        }
        std::map<uint32_t, std::shared_ptr<Sampler>> samplers;
        for (auto&& description : pipeline->sampler_layout_.descriptions_) {
            auto it = draw_command->samplers_.find(description.binding);
            if (it == draw_command->samplers_.end()) {
                logger().error("Cannot find sampler for binding {} in draw command {}", description.binding, draw_command->name());
            } else {
                samplers[description.binding] = it->second;
            }
        }
        vk::DescriptorSet descriptor_set = nullptr;
        if (!render_target_pipeline_resources.descriptor_sets.empty()) {
            descriptor_set = *render_target_pipeline_resources.descriptor_sets[i];
        }

        resources->draw_command_resources.back().emplace_back(
            RenderTargetDrawCommandResources{
                .pipeline        = *render_target_pipeline_resources.pipeline,
                .pipeline_layout = *pipeline_resources->pipeline_layout,
                .descriptor_set  = descriptor_set,
                .uniforms        = std::move(gpu_uniforms),
                .samplers        = std::move(samplers),
            }
        );
    }

    for (size_t i = 0; i < image_count; ++i) {
        auto& framebuffer_resources = resources->framebuffer_resources[i];
        auto& draw_command_resources = resources->draw_command_resources.back()[i];
        std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
        std::vector<std::vector<vk::DescriptorBufferInfo>> buffer_infos;
        std::vector<std::vector<vk::DescriptorImageInfo>> image_infos;

        for (auto&& description : pipeline->uniform_layout_.descriptions_) {
            // Find GPU uniform data
            const auto* gpu_uniform = [description, &framebuffer_resources, &draw_command_resources]()
                -> const std::shared_ptr<UniformBufferBase>* {
                for (auto&& uniform_buffer : framebuffer_resources.uniforms) {
                    if (description.attribute == uniform_buffer->description().attribute) {
                        return &uniform_buffer;
                    }
                }
                for (auto&& uniform_buffer : draw_command_resources.uniforms) {
                    if (description.attribute == uniform_buffer->description().attribute) {
                        return &uniform_buffer;
                    }
                }
                return nullptr;
            }();

            std::vector<vk::DescriptorBufferInfo> buffer_info;
            if (auto* buffer_resources = (*gpu_uniform)->impl_->resources.data()) {
                buffer_info.emplace_back(
                    vk::DescriptorBufferInfo{
                        .buffer = *buffer_resources->buffer,
                        .offset = 0,
                        .range  = (*gpu_uniform)->data_size()
                    }
                );
            } else {
                logger().error("Uniform buffer resources not available.");
            }
            buffer_infos.emplace_back(std::move(buffer_info));
        }

        for (size_t j = 0; j < pipeline->uniform_layout_.descriptions_.size(); ++j) {
            auto& description = pipeline->uniform_layout_.descriptions_[j];
            auto write_description_set = vk::WriteDescriptorSet{
                .dstSet          = *render_target_pipeline_resources.descriptor_sets[i],
                .dstBinding      = description.binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer
            }
                .setBufferInfo(buffer_infos[j]);
            write_descriptor_sets.emplace_back(std::move(write_description_set));
        }

        for (size_t j = 0; j < pipeline->sampler_layout_.descriptions_.size(); ++j) {
            auto& description = pipeline->sampler_layout_.descriptions_[j];
            std::vector<vk::DescriptorImageInfo> image_info;
            if (draw_command_resources.samplers.find(description.binding) == draw_command_resources.samplers.end()) {
                image_infos.emplace_back(std::move(image_info));
                continue;
            }

            auto&& sampler = draw_command_resources.samplers[description.binding];
            if (auto* image_resources = sampler->image_->impl_->resources.data()) {
                if (auto* sampler_resources = sampler->impl_->resources.data()) {
                    image_info.emplace_back(
                        vk::DescriptorImageInfo{
                            .sampler     = *sampler_resources->sampler,
                            .imageView   = *image_resources->image_view,
                            .imageLayout = image_resources->image_layout
                        }
                    );
                    image_infos.emplace_back(std::move(image_info));
                }
            } else {
                logger().error("Image/sampler resources not available.");
            }
        }

        for (size_t j = 0; j < pipeline->sampler_layout_.descriptions_.size(); ++j) {
            auto& description = pipeline->sampler_layout_.descriptions_[j];
            if (image_infos[j].empty()) {
                continue;
            }
            auto write_description_set = vk::WriteDescriptorSet{
                .dstSet          = *render_target_pipeline_resources.descriptor_sets[i],
                .dstBinding      = description.binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
            }
                .setImageInfo(image_infos[j]);
            write_descriptor_sets.emplace_back(std::move(write_description_set));
        }

        logical_device_->impl_->vk_device.updateDescriptorSets(write_descriptor_sets, {});
    }
}

void Gfx::commitDrawCommandUniformBuffers(
    const std::shared_ptr<RenderTarget>& render_target, const std::shared_ptr<DrawCommand>& draw_command,
    uniform_attributes::UniformAttribute specified_attribute,
    int image_index
) {

    auto& renderer = render_target->renderer_;

    if (!renderer) {
        logger().error("Cannot commit draw command uniform buffer because renderer is not available.");
        return;
    }

    size_t draw_command_index = renderer->getDrawCommandIndex(draw_command);
    if (draw_command_index == SIZE_MAX) {
        logger().error("Cannot commit draw command uniform buffer because it is not in render target resources.");
        return;
    }

    commitDrawCommandUniformBuffers(render_target, draw_command_index, specified_attribute, image_index);
}

void Gfx::commitDrawCommandUniformBuffers(
    const std::shared_ptr<RenderTarget>& render_target, size_t draw_command_index,
    uniform_attributes::UniformAttribute specified_attribute,
    int image_index
) {

    auto& renderer = render_target->renderer_;

    if (!renderer) {
        logger().error("Cannot commit draw command uniform buffer because renderer is not available.");
        return;
    }

    if (!logical_device_) {
        logger().error("Cannot commit draw command uniform buffer because logical device is not available.");
        return;
    }

    auto* resources = render_target->impl_->resources.data();
    if (!resources) {
        logger().error("Cannot commit draw command uniform buffer because render target resources are not available.");
        return;
    }

    const auto& draw_command = renderer->getDrawCommands()[draw_command_index];

    size_t image_count = resources->framebuffer_resources.size();
    int start_index = image_index >= 0 ? image_index : 0;
    int end_index = image_index >= 0 ? image_index + 1U : static_cast<int>(image_count);
    for (int i = start_index; i < end_index; ++i) {
        auto& draw_command_resources = resources->draw_command_resources[draw_command_index][i];
        for (auto&&[attribute, cpu_uniform] : draw_command->uniform_buffers_) {
            if (specified_attribute == uniform_attributes::none || specified_attribute == attribute) {
                // Find GPU uniform data
                const auto* gpu_uniform = [attrib = attribute, &draw_command_resources]()
                    -> const std::shared_ptr<UniformBufferBase>* {
                    for (auto&& uniform_buffer : draw_command_resources.uniforms) {
                        if (attrib == uniform_buffer->description().attribute) {
                            return &uniform_buffer;
                        }
                    }
                    return nullptr;
                }();
                commitReferenceBuffer(cpu_uniform, *gpu_uniform);
            }
        }
    }
}

} // namespace wg