#include "gfx/gfx.h"
#include "engine/mesh-component.h"
#include "common/logger.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void MeshComponentRenderData::createGfxResources(Gfx& gfx) {
    for (auto&& draw_command : draw_commands) {
        gfx.finishDrawCommand(draw_command);
    }
}

std::shared_ptr<IRenderData> MeshComponent::createRenderData() {
    render_data_ = std::shared_ptr<MeshComponentRenderData>(new MeshComponentRenderData());
    auto material_render_data_ = material_->render_data();
    render_data_->model_uniform_buffer = UniformBuffer<ModelUniform>::Create();
    render_data_->model_uniform_buffer->setUniformObject(createUniformObject());
    render_data_->draw_commands = { SimpleDrawCommand::Create(name_, material_render_data_->pipeline) };
    for (auto&& draw_command : render_data_->draw_commands) {
        draw_command->addVertexBuffer(mesh_->render_data()->vertex_buffer);
        if (mesh_->render_data()->index_buffer) {
            draw_command->setIndexBuffer(mesh_->render_data()->index_buffer);
        }
        draw_command->addUniformBuffer(render_data_->model_uniform_buffer);
    }
    return render_data_;
}

ModelUniform MeshComponent::createUniformObject() {
    return { .model_mat = transform_.transform };
}

} // namespace wg