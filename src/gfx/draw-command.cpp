#include "gfx/gfx.h"
#include "gfx/draw-command.h"
#include "gfx/gfx-buffer.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "draw-command-private.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

DrawCommand::DrawCommand(std::string name)
    : name_(std::move(name)) {}

void DrawCommand::setPipeline(const std::shared_ptr<GfxPipeline>& pipeline) {
    pipeline_ = pipeline;
}

void DrawCommand::addVertexBuffer(const std::shared_ptr<VertexBufferBase>& vertex_buffer) {
    vertex_buffers_.emplace_back(vertex_buffer);
}

void DrawCommand::clearVertexBuffers() {
    vertex_buffers_.clear();
}

void DrawCommand::setIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer) {
    index_buffer_ = index_buffer;
}

void DrawCommand::clearIndexBuffer() {
    index_buffer_.reset();
}

bool DrawCommand::draw_indexed() const {
    return static_cast<bool>(index_buffer_);
}

size_t DrawCommand::vertex_count() const {
    if (vertex_buffers_.empty()) {
        return 0;
    }
    size_t result = std::numeric_limits<size_t>::max();
    for (auto&& vertex_buffer : vertex_buffers_) {
        result = std::min(result, vertex_buffer->vertex_count());
    }
    return result;
}

size_t DrawCommand::index_count() const {
    if (index_buffer_) {
        return index_buffer_->index_count();
    }
    return 0;
}

std::vector<VertexBufferCombinedDescription> DrawCommand::getVertexBufferCombinedDescriptions() const {
    if (!pipeline_) {
        return {};
    }

    std::vector<VertexBufferDescription> vertex_buffer_descriptions;
    for (auto&& vertex_buffer : vertex_buffers_) {
        for (auto&& description : vertex_buffer->descriptions()) {
            AddDescriptionByAttributeImplTemplate(vertex_buffer_descriptions, description);
        }
    }
    std::vector<VertexBufferCombinedDescription> combined_descriptions;
    for (auto&& description : pipeline_->vertex_factory().descriptions()) {
        for (size_t vertex_buffer_index = 0; vertex_buffer_index < vertex_buffers_.size(); ++vertex_buffer_index) {
            const auto& vertex_buffer = vertex_buffers_[vertex_buffer_index];
            for (auto&& vertex_buffer_description : vertex_buffer->descriptions()) {
                if (vertex_buffer_description.attribute == description.attribute &&
                    vertex_buffer_description.format == description.format) {

                    combined_descriptions.emplace_back(
                        VertexBufferCombinedDescription{
                            .attribute           = description.attribute,
                            .format              = description.format,
                            .location            = description.location,
                            .stride              = vertex_buffer_description.stride,
                            .offset              = vertex_buffer_description.offset,
                            .vertex_buffer_index = vertex_buffer_index,
                        }
                    );
                }
            }
        }
    }
    return combined_descriptions;
}

DrawCommand& DrawCommand::addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    uniform_buffers_[uniform_buffer->description().attribute] = uniform_buffer;
    return *this;
}

void DrawCommand::clearUniformBuffers() {
    uniform_buffers_.clear();
}

DrawCommand& DrawCommand::addImage(uint32_t binding, const std::shared_ptr<Image>& image) {
    images_[binding] = image;
    return *this;
}

void DrawCommand::clearImages() {
    images_.clear();
}

bool DrawCommand::valid() const {
    if (!pipeline_) {
        return false;
    }

    std::vector<VertexBufferDescription> vertex_buffer_descriptions;
    for (auto&& vertex_buffer : vertex_buffers_) {
        for (auto&& description : vertex_buffer->descriptions()) {
            AddDescriptionByAttributeImplTemplate(vertex_buffer_descriptions, description);
        }
    }
    for (auto&& description : pipeline_->vertex_factory().descriptions()) {
        auto it = std::lower_bound(
            vertex_buffer_descriptions.begin(), vertex_buffer_descriptions.end(), description,
            [](const VertexBufferDescription& element, const VertexFactoryDescription& value) {
                return element.attribute < value.attribute;
            }
        );
        if (it == vertex_buffer_descriptions.end() ||
            it->attribute != description.attribute ||
            it->format != description.format) {
            return false;
        }
    }

    for (auto&& description : pipeline_->uniform_layout().descriptions()) {
        if (description.attribute >= uniform_attributes::DRAW_COMMAND_UNIFORMS_START &&
            description.attribute < uniform_attributes::DRAW_COMMAND_UNIFORMS_END) {
            if (uniform_buffers_.find(description.attribute) == uniform_buffers_.end()) {
                return false;
            }
        }
    }

    for (auto&& description : pipeline_->sampler_layout().descriptions()) {
        if (images_.find(description.binding) == images_.end()) {
            return false;
        }
    }

    return true;
}

void Gfx::finishDrawCommand(const std::shared_ptr<DrawCommand>& draw_command) {
    auto impl = draw_command->getImpl();

    // Vertex factory
    impl->vertex_count = static_cast<uint32_t>(draw_command->vertex_count());
    std::map<size_t, uint32_t> vb_index_to_binding;
    for (auto&& description : draw_command->getVertexBufferCombinedDescriptions()) {
        uint32_t binding = [impl, &vb_index_to_binding, &description]() {
            auto it = vb_index_to_binding.find(description.vertex_buffer_index);
            if (it == vb_index_to_binding.end()) {
                uint32_t new_binding = static_cast<uint32_t>(impl->vertex_bindings.size());
                vb_index_to_binding[description.vertex_buffer_index] = new_binding;
                impl->vertex_bindings.emplace_back(
                    vk::VertexInputBindingDescription{
                        .binding = new_binding,
                        .stride = description.stride,
                        .inputRate = vk::VertexInputRate::eVertex
                    }
                );
                return new_binding;
            } else {
                if (impl->vertex_bindings[it->second].stride != description.stride) {
                    logger().warn(
                        "Vertex binding and buffer stride not identical: {} != {}.",
                        impl->vertex_bindings[it->second].stride, description.stride
                    );
                }
                return it->second;
            }
        }();

        impl->vertex_attributes
            .emplace_back(
                vk::VertexInputAttributeDescription{
                    .location = description.location,
                    .binding = binding,
                    .format = gfx_formats::ToVkFormat(description.format),
                    .offset = description.offset
                }
            );

        auto&& vertex_buffer = draw_command->vertex_buffers()[description.vertex_buffer_index];
        if (auto* vertex_buffer_resources = vertex_buffer->impl_->resources.data()) {
            impl->vertex_buffers.emplace_back(*vertex_buffer_resources->buffer);
            impl->vertex_buffer_offsets
                .emplace_back(0);
        }
    }

    impl->draw_indexed = draw_command->draw_indexed();
    if (impl->draw_indexed) {
        if (auto* index_buffer_resources = draw_command->index_buffer_->impl_->resources.data()) {
            impl->index_buffer = *index_buffer_resources->buffer;
        }
        impl->index_buffer_offset = 0;
        impl->index_type = index_types::ToVkIndexType(draw_command->index_buffer_->index_type());
        impl->index_count = static_cast<uint32_t>(draw_command->index_count());
    }

    impl->vertex_input_create_info = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(impl->vertex_bindings)
        .setVertexAttributeDescriptions(impl->vertex_attributes);

    impl->input_assembly_create_info = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false
    };
}

std::shared_ptr<DrawCommand> SimpleDrawCommand::Create(
    std::string name, const std::shared_ptr<GfxPipeline>& pipeline
) {
    return std::shared_ptr<DrawCommand>(new SimpleDrawCommand(std::move(name), pipeline));
}

SimpleDrawCommand::SimpleDrawCommand(std::string name, const std::shared_ptr<GfxPipeline>& pipeline)
    : DrawCommand(std::move(name)), impl_(std::make_unique<Impl>()) {
    pipeline_ = pipeline;
}

DrawCommand::Impl* SimpleDrawCommand::getImpl() {
    return impl_.get();
}

void SimpleDrawCommand::Impl::draw(vk::CommandBuffer& command_buffer) {
    command_buffer.bindVertexBuffers(0, vertex_buffers, vertex_buffer_offsets);
    if (draw_indexed) {
        command_buffer.bindIndexBuffer(
            index_buffer, index_buffer_offset, index_type
        );
        command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
    } else {
        command_buffer.draw(vertex_count, 1, 0, 0);
    }
}

} // namespace wg