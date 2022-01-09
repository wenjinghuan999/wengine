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

void DrawCommand::setPipeline(const std::shared_ptr<GfxPipeline>& pipeline) {
    auto* resources = pipeline->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot set pipeline for draw command because pipeline resources are not available.");
        return;
    }

    pipeline_ = pipeline;
    getImpl()->pipeline_resources = resources;
}

std::shared_ptr<DrawCommand> SimpleDrawCommand::Create(
    const std::shared_ptr<GfxPipeline>& pipeline
) {
    return std::shared_ptr<DrawCommand>(new SimpleDrawCommand(pipeline));
}

SimpleDrawCommand::SimpleDrawCommand(const std::shared_ptr<GfxPipeline>& pipeline)
    : DrawCommand(), impl_(std::make_unique<Impl>())  {
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

