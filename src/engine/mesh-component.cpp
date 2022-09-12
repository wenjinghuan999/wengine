#include "engine/mesh-component.h"

#include "common/logger.h"
#include "gfx/gfx.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void MeshComponentRenderData::createGfxResources(Gfx& gfx) {
    for (auto&& draw_command : draw_commands) {
        if (!draw_command->valid()) {
            logger().error("Skip create resources for mesh component because draw command is invalid.");
            continue;
        }
        gfx.finishDrawCommand(draw_command);
    }
}

std::shared_ptr<IRenderData> MeshComponent::createRenderData() {
    render_data_ = std::shared_ptr<MeshComponentRenderData>(new MeshComponentRenderData());
    auto material_render_data_ = material_->render_data();
    render_data_->model_uniform_buffer = UniformBuffer<ModelUniform>::Create();
    render_data_->model_uniform_buffer->setUniformObject(createUniformObject());
    render_data_->draw_commands = { SimpleDrawCommand::Create(name_, material_render_data_->pipeline) };
    render_data_->draw_commands[0]->setPrimitiveTopology(mesh_->primitive_topology());
    for (auto&& draw_command : render_data_->draw_commands) {
        draw_command->addVertexBuffer(mesh_->render_data()->vertex_buffer);
        if (mesh_->render_data()->index_buffer) {
            draw_command->setIndexBuffer(mesh_->render_data()->index_buffer);
        }
        draw_command->addUniformBuffer(render_data_->model_uniform_buffer);
        for (size_t i = 0; i < material_render_data_->pipeline->sampler_layout().descriptions().size(); ++i) {
            if (i < material_->textures().size()) {
                auto&& description = material_render_data_->pipeline->sampler_layout().descriptions()[i];
                auto&& sampler = material_->render_data()->samplers[i];
                draw_command->addSampler(description.binding, sampler);
            }
        }
    }
    return render_data_;
}

ModelUniform MeshComponent::createUniformObject() {
    return { .model_mat = transform_.transform };
}

} // namespace wg