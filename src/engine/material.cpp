#include "gfx/gfx.h"
#include "engine/material.h"
#include "common/logger.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void MaterialRenderData::createGfxResources(Gfx& gfx) {
    for (auto&& shader : pipeline->shaders()) {
        gfx.createShaderResources(shader);
    }
    gfx.createPipelineResources(pipeline);
}

std::shared_ptr<Material> Material::Create(
    const std::string& name, const std::string& vert_shader_filename, const std::string& frag_shader_filename
) {
    return std::shared_ptr<Material>(new Material(name, vert_shader_filename, frag_shader_filename));
}

Material::Material(
    const std::string& name,
    const std::string& vert_shader_filename, const std::string& frag_shader_filename
) : name_(name), vert_shader_filename_(vert_shader_filename), frag_shader_filename_(frag_shader_filename) {
}

std::shared_ptr<IRenderData> Material::createRenderData() {
    render_data_ = std::shared_ptr<MaterialRenderData>(new MaterialRenderData());

    auto vert_shader = Shader::Load(vert_shader_filename_, shader_stages::vert);
    auto frag_shader = Shader::Load(frag_shader_filename_, shader_stages::frag);

    render_data_->pipeline = GfxPipeline::Create();
    render_data_->pipeline->setVertexFactory(createVertexFactory());
    render_data_->pipeline->setUniformLayout(createUniformLayout());
    render_data_->pipeline->addShader(vert_shader);
    render_data_->pipeline->addShader(frag_shader);

    return render_data_;
}

GfxVertexFactory Material::createVertexFactory() const {
    return {
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
    };
}

GfxUniformLayout Material::createUniformLayout() const {
    return wg::GfxUniformLayout{}
        .addDescription({ .attribute = wg::uniform_attributes::camera, .binding = 0, .stages = wg::shader_stages::vert | wg::shader_stages::frag })
        .addDescription({ .attribute = wg::uniform_attributes::model, .binding = 1, .stages = wg::shader_stages::vert | wg::shader_stages::frag });
}

} // namespace wg