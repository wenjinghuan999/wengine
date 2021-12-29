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

DrawCommand::DrawCommand(const std::shared_ptr<GfxPipeline>& pipeline)
    : pipeline_(pipeline), impl_(std::make_unique<Impl>()) {}

std::shared_ptr<DrawCommand> SimpleDrawCommand::Create(
    const std::shared_ptr<GfxPipeline>& pipeline
) {
    return std::shared_ptr<DrawCommand>(new SimpleDrawCommand(pipeline));
}

SimpleDrawCommand::SimpleDrawCommand(const std::shared_ptr<GfxPipeline>& pipeline)
    : DrawCommand(pipeline) {
    
    impl_->draw = [this](const vk::CommandBuffer& command_buffer) {
        if (pipeline_->vertex_factory().draw_indexed()) {
            uint32_t index_count = static_cast<uint32_t>(pipeline_->vertex_factory().index_count());
            command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
        } else {
            uint32_t vertex_count = static_cast<uint32_t>(pipeline_->vertex_factory().vertex_count());
            command_buffer.draw(vertex_count, 1, 0, 0);
        }
    };
}

} // namespace wg

