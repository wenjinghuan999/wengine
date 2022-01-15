#include "gfx/gfx.h"
#include "engine/scene-renderer.h"
#include "common/logger.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void SceneRendererRenderData::createGfxResources(Gfx& gfx) {
    gfx.createRenderTargetResources(render_target);
    gfx.submitDrawCommands(render_target);
}

std::shared_ptr<IRenderData> SceneRenderer::createRenderData() {
    render_target_->setRenderer(shared_from_this());
    for (auto&& component : components_) {
        if (component->render_data()) {
            auto& draw_commands = component->render_data()->draw_commands;
            std::copy(draw_commands.begin(), draw_commands.end(), std::back_inserter(draw_commands_));
        }
    }
    
    render_data_ = std::shared_ptr<SceneRendererRenderData>(new SceneRendererRenderData());
    render_data_->render_target = render_target_;
    render_data_->camera_uniform_buffer = UniformBuffer<CameraUniform>::Create();
    render_data_->camera_uniform_buffer->setUniformObject(createUniformObject());
    addUniformBuffer(render_data_->camera_uniform_buffer);
    
    return render_data_;
}

CameraUniform SceneRenderer::createUniformObject() {
    return {
        .view_mat = glm::lookAt(camera_.position, camera_.center, camera_.up),
        .project_mat = glm::perspective(glm::radians(45.0f), 4.f / 3.f, 0.1f, 10.0f)
    };
}

} // namespace wg