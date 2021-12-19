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

SimpleDrawCommand::SimpleDrawCommand(const std::shared_ptr<GfxPipeline>& pipeline)
    : DrawCommand(pipeline) {
    
    impl_->draw = [](const vk::CommandBuffer& command_buffer) {
        command_buffer.draw(3, 1, 0, 0);
    };
}

} // namespace wg

