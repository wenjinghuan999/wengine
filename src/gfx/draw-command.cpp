#include "gfx/gfx.h"
#include "gfx/draw-command.h"
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
    auto* resources = pipeline->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot set pipeline for draw command because pipeline resources are not available.");
        return;
    }

    pipeline_ = pipeline;
    getImpl()->pipeline_resources = resources;
}

DrawCommand& DrawCommand::addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    uniform_buffers_[uniform_buffer->description().attribute] = uniform_buffer;
    return *this;
}

void DrawCommand::clearUniformBuffers() {
    uniform_buffers_.clear();
}

bool DrawCommand::valid() const {
    if (!pipeline_) {
        return false;
    }
    for (auto&& description : pipeline_->uniform_layout().descriptions()) {
        if (description.attribute >= uniform_attributes::DRAW_COMMAND_UNIFORMS_START &&
            description.attribute < uniform_attributes::DRAW_COMMAND_UNIFORMS_END) {
            if (uniform_buffers_.find(description.attribute) == uniform_buffers_.end()) {
                return false;
            }
        }
    }
    return true;
}

std::shared_ptr<DrawCommand> SimpleDrawCommand::Create(
    std::string name, const std::shared_ptr<GfxPipeline>& pipeline
) {
    return std::shared_ptr<DrawCommand>(new SimpleDrawCommand(std::move(name), pipeline));
}

SimpleDrawCommand::SimpleDrawCommand(std::string name, const std::shared_ptr<GfxPipeline>& pipeline)
    : DrawCommand(std::move(name)), impl_(std::make_unique<Impl>())  {
    setPipeline(pipeline);
}

DrawCommand::Impl* SimpleDrawCommand::getImpl() {
    return impl_.get();
}

void SimpleDrawCommand::Impl::draw(vk::CommandBuffer& command_buffer) {
    command_buffer.bindVertexBuffers(0, pipeline_resources->vertex_buffers, pipeline_resources->vertex_buffer_offsets);
    if (pipeline_resources->draw_indexed) {
        command_buffer.bindIndexBuffer(
            pipeline_resources->index_buffer, pipeline_resources->index_buffer_offset, pipeline_resources->index_type);
    }

    if (pipeline_resources->draw_indexed) {
        command_buffer.drawIndexed(pipeline_resources->index_count, 1, 0, 0, 0);
    } else {
        command_buffer.draw(pipeline_resources->vertex_count, 1, 0, 0);
    }
}

} // namespace wg

